
#
# Microcontroller Console
# Version v0.1a
# Written by Dave Rajnauth, VE3OOI
#
# Licensed under a Creative Commons Attribution 4.0 International License.
# No Commercial Use
#

# Need to install the following libraries to run this python script.
# Execute the following python commands:
# python -m pip install -U pip
# python - m pip install - U matplotlib
# python - m pip install -U numpy
# python - m pip install -U serial


##################################################
# Imports
##################################################
import tkinter as tk
from tkinter import *  # filedialog, Text, Label, Entry, Button
from tkinter import messagebox
from tkinter import filedialog
from PIL import ImageTk, Image
import matplotlib
from matplotlib import pyplot as plt
from matplotlib.animation import FuncAnimation
import csv
import math
import numpy as np
import threading
import serial
import serial.tools.list_ports
import time
import os
import sys

##################################################
# Global variables
##################################################
# System and Window ontrol
global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
global win, thd, disableAnimate
# Serial port ontrol
global ser, serialLoop
# Data variables
global cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
# Widgets
global entButton, cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
global comPortOption, baudRateOption, connectButton

screenHeight = 600
screenWidth = 600
nrows = 10
ncolumns = 20
flags = 0
banner = "VE3OOI Microcontroller PC Interface v0.1c"


def resource_path(relative_path):
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")

    return os.path.join(base_path, relative_path)


##################################################
# Window Creation
##################################################
root = tk.Tk()

root.geometry(f"{screenWidth}x{screenHeight}")
root.title(banner)


photo = ImageTk.PhotoImage(
    Image.open(resource_path("Shack_Icon.png")))
root.iconphoto(False, photo)
root.geometry('600x520')
root.resizable(False, False)

# Make Grids responsive
for i in range(nrows):
    root.grid_rowconfigure(i,  weight=1)
for i in range(ncolumns):
    root.grid_columnconfigure(i, weight=1)

win = root
##################################################
# Routines
##################################################


def mainCode():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    cmd = tk.StringVar()

    lab1 = tk.Label(win, text="Command Entry",
                    fg='blue', relief=RAISED)
#    lab1.place(x=10, y=10, height=15)
    lab1.grid(row=0, column=0, columnspan=3, padx=5, ipadx=5, sticky="W")

    entButton = tk.Button(win, text="Enter", command=entButtonHandler)
#    entButton.place(x=xval, y=25)
    entButton.grid(row=1, column=0, padx=3, ipadx=3, sticky="W")
    entButton.configure(fg='black', bg='red', state=tk.DISABLED)

    cmdValidation = win.register(cmdCallback)
    cmdEntry = tk.Entry(win, width=70, textvariable=cmd, validate="key", validatecommand=(
        cmdValidation, '%d', '%S', '%s', '%P', '%i'), fg='black', bg='white')
    cmdEntry.grid(row=1, column=1, columnspan=ncolumns, pady=3, sticky="W")
    cmdEntry.configure(state=tk.DISABLED)

    lab2 = tk.Label(win, text="uC Output", fg='blue',
                    relief=RAISED)
    lab2.grid(row=2, column=0, columnspan=3, padx=5, pady=1, sticky="W")

    txtEntry = Text(win, width=70,  wrap="none")
    txtEntry.tag_configure('note', foreground='blue')
    txtEntry.tag_configure('error', foreground='red')
    # Make field readonly but allow copying
    txtEntry.bind("<Key>", lambda e: textEvent(e))
    txtEntryLastRow = nrows-6
    txtEntry.grid(row=3, column=0, columnspan=ncolumns-2,
                  rowspan=txtEntryLastRow, padx=2, pady=2, ipadx=2, sticky="NW")
    txtEntry.delete(1.0, END)

    vScroll = Scrollbar(win)
    txtEntry.config(yscrollcommand=vScroll.set)
    vScroll.config(command=txtEntry.yview)
    vScroll.grid(column=ncolumns-2, row=(2 + txtEntryLastRow//2),
                 rowspan=10, ipady=80, sticky='NW')

    hScroll = Scrollbar(win, orient=HORIZONTAL)
    txtEntry.config(xscrollcommand=hScroll.set)
    hScroll.config(command=txtEntry.xview)
    hScroll.grid(row=txtEntryLastRow+4, column=ncolumns //
                 2-8, columnspan=6, ipadx=80, sticky='NW')

    txtEntryLastRow += 5  # Text box starts at 2 and need 1 space

    exitButton = tk.Button(win, text="Exit", command=terminate)
    exitButton.grid(row=txtEntryLastRow, column=0,
                    padx=2, ipadx=2, sticky="W")

    csvText = BooleanVar()
    csvText.set(False)
    csvButton = Checkbutton(win, text="Save CSV", variable=csvText,
                            onvalue=True, offvalue=False, command=csvHandler)
    csvButton.grid(row=txtEntryLastRow, column=1,
                   padx=4, pady=10, sticky="W")

    plot = BooleanVar()
    plot.set(False)
    plotButton = Checkbutton(win, text="Plot", variable=plot,
                             onvalue=True, offvalue=False, command=plotHandler)
    plotButton.grid(row=txtEntryLastRow, column=2,
                    padx=0, pady=10, sticky="W")

    plotType = [CONST.TYPE_XY, CONST.TYPE_SXY,
                CONST.TYPE_Y, CONST.TYPE_LIVEXY, CONST.TYPE_LIVEY]
    plotTypeString = tk.StringVar(win)
    plotTypeString.set(plotType[0])  # default value
    plotTypeOption = OptionMenu(
        win, plotTypeString, *plotType, command=plotOptionHandler)
    plotTypeOption.grid(row=txtEntryLastRow, column=3,
                        padx=4, pady=10, ipadx=6,  sticky="W")

    executeButton = tk.Button(win, text="GO", command=execute)
    executeButton.grid(row=txtEntryLastRow, column=4,
                       padx=2, ipadx=2, sticky="W")

    enumerateComPorts()

    baudRates = ["9600", "19200", "38400", "57600", "115200", "230400"]
    baudRateString = tk.StringVar(win)
    baudRateString.set(baudRates[0])  # default value
    baudRateOption = OptionMenu(win, baudRateString, *baudRates)
    baudRateOption.grid(row=txtEntryLastRow, column=6,
                        padx=4, pady=10, ipadx=4,  sticky="W")

    connected = BooleanVar()
    connected.set(False)
    connectButton = Checkbutton(win, text="Connect", variable=connected,
                                onvalue=True, offvalue=False, command=connectHandler)
    connectButton.grid(row=txtEntryLastRow, column=7,
                       padx=4, pady=10, sticky="W")

    plotButton.configure(state=tk.DISABLED)
    csvButton.configure(state=tk.DISABLED)
    plotTypeOption.configure(state=tk.DISABLED)
    executeButton.configure(state=tk.DISABLED)


def enumerateComPorts():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    serialPorts = serial.tools.list_ports.comports()
    portListing = []
    txtEntry.insert(tk.END, "Enumerating serial ports...\r\n", ('note'))
    for port, desc, hwid in sorted(serialPorts):
        txtEntry.insert(
            tk.END, "   Port: {}: Desc: {}\r\n".format(port, desc), ('note'))
        portListing.append(port)

    if len(portListing) < 1:
        txtEntry.insert(
            tk.END,   "Fatal Error No Com Ports Found - uncomment code", ('note'))
        messagebox.showerror(
            "Fatal Error", "No Com Ports Found")
        terminate()

    comPortString = tk.StringVar(win)
    comPortString.set(portListing[0])  # default value
    comPortOption = OptionMenu(win, comPortString, *portListing)
    comPortOption.grid(row=txtEntryLastRow, column=5,
                       padx=4, pady=10, ipadx=4, sticky="W")


def execute():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    if (data.size == 0):
        txtEntry.insert(
            tk.END, "Insufficient data captured. Cannot complete" + "\r\n", ('error'))
        txtEntry.see("end")
        plot.set(False)
        return

    if plot.get():
        if plotTypeString.get() == CONST.TYPE_XY:
            x, y = zip(*data)

            plt.plot(x, y, color='green', linewidth=2,
                     linestyle='-', label='uC Series 1')

            plt.title('uController Data Plot')
            plt.ylabel('Y Values')
            plt.xlabel('X Values')

            if float(min(x)) < float(max(x)) and float(min(y)) < float(max(y)):
                plt.xticks(np.arange(float(min(x)), float(max(x)), (float(
                    max(x))-float(min(x))) / 10), rotation='vertical')
                plt.yticks(np.arange(float(min(y)), float(max(y)),
                           (float(max(y))-float(min(y))) / 10))

            if csvText.get():
                writeCSV(data)

            plt.legend()
            plt.grid()
            plt.tight_layout()
            plt.show()

        elif plotTypeString.get() == CONST.TYPE_SXY:
            plt.title('uController Data Plot')
            plt.ylabel('Y Values')
            plt.xlabel('X Values')
            nrows, ncols = data.shape

            minx = 65535
            maxx = 0
            miny = 65535
            maxy = 0

            colors = matplotlib.colors.cnames.items()
            colors = sorted(colors, key=lambda x: (x[1]))
            tempdata = np.empty((0, 3))
            newdata = np.empty((0, 3))

            for i in range(0, ncols, 3):
                s = np.array(data[:, i])
                x = np.array(data[:, i+1])
                y = np.array(data[:, i+2])

                if max(x) > maxx:
                    maxx = max(x)
                if max(y) > maxy:
                    maxy = max(y)
                if min(x) < minx:
                    minx = min(x)
                if min(y) < miny:
                    miny = min(y)

                if np.count_nonzero(s) > 0:
                    seriestxt = str(round_sig(np.mean(s), 4))
                else:
                    seriestxt = "0"
                plt.plot(x, y, color=str(
                    colors[i][1]), linewidth=2, linestyle='-', label=seriestxt)

                if csvText.get():
                    newdata = np.append(
                        s.reshape(s.size, 1), x.reshape(s.size, 1), axis=1)
                    newdata = np.append(newdata, y.reshape(s.size, 1), axis=1)
                    writeCSV(newdata)
                    fs = open(fileName, 'a+', newline='')
                    fs.write("\r\n")
                    fs.close()
                    newdata = np.empty((0, 3))

            plt.xticks(np.arange(minx, maxx, (maxx-minx) / 10),
                       rotation='vertical')
            plt.yticks(np.arange(miny, maxy, (maxy-miny) / 10))

            plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
            plt.grid()
            plt.tight_layout()
            plt.show()

        elif plotTypeString.get() == CONST.TYPE_Y:
            y = data
            x = np.arange(0, y.size)

            plt.plot(x, y, color='green', linewidth=2,
                     linestyle='-', marker="o", label='uC Series 1')

            plt.title('uController Data Plot')
            plt.ylabel('Y Values')
            plt.xlabel('X Values')

            plt.xticks(np.arange(0, max(x), (max(x) // 10)),
                       rotation='vertical')
            plt.yticks(np.arange(float(min(y)), float(max(y)),
                       (float(max(y))-float(min(y))) / 10))

            if csvText.get():
                writeCSV(data)

            plt.legend()
            plt.grid()
            plt.tight_layout()
            plt.show()

    csvText.set(False)
    plot.set(False)
    executeButton.configure(state=tk.DISABLED)
    data = np.array([])


def animate(i):
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    try:
        if (data.size == 0 or disableAnimate):
            return

        if plot.get():
            disableAnimate = True
            if plotTypeString.get() == CONST.TYPE_LIVEXY:
                x, y = zip(*data)
                plt.plot(x, y, color='green', linewidth=2,
                         linestyle='-', marker="o", label='Live Series')

            elif plotTypeString.get() == CONST.TYPE_LIVEY:
                y = data
                x = np.arange(0, y.size)

                plt.plot(y, color='blue', linewidth=2, linestyle='-',
                         marker="o", label='Live Series')

        if csvText.get():
            writeCSV(data)

    except Exception as e:
        txtEntry.insert(tk.END, "Exception: ", ('note'))
        txtEntry.insert(tk.END, e,  ('error'))
        txtEntry.insert(tk.END, "  \r\n", ('note'))
        txtEntry.see("end")


def textEvent(event):
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    if(event.state == 12 and event.keysym == 'c'):
        return
    else:
        return "break"


def terminate():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton
    try:
        serialLoop = False
        if ser.is_open:
            ser.close()

        if thd.is_alive():
            thd.join()

        win.quit()
        sys.exit()
    except:
        sys.exit()


def plotOptionHandler(args):
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate, ani
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    if plot.get():
        plt.close('all')
        csvText.set(False)
        plot.set(False)
        executeButton.configure(state=tk.DISABLED)
        data = np.array([])


def plotHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate, ani
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    data = np.array([])
    series = 0

    if plot.get():
        #        loadArray() #added to test
        txtEntry.insert(tk.END, "Plotting Enabled for " +
                        plotTypeString.get() + " Data\r\n", ('note'))
        txtEntry.see("end")

        if plotTypeString.get() == CONST.TYPE_LIVEXY or plotTypeString.get() == CONST.TYPE_LIVEY:
            try:
                fig = plt.figure()
                plt.title('uController Data Plot')
                plt.ylabel('Y Values')
                plt.xlabel('X Values')

                if plotTypeString.get() == CONST.TYPE_LIVEXY:
                    plt.plot([0], [0], color='green', linewidth=2,
                             linestyle='-', marker="o", label='Live Series')

                elif plotTypeString.get() == CONST.TYPE_LIVEY:
                    plt.plot([0], color='blue', linewidth=2,
                             linestyle='-', marker="o", label='Live Series')

                plt.grid()
                plt.tight_layout()
                disableAnimate = True
                ani = matplotlib.animation.FuncAnimation(
                    fig, animate, interval=500)
                plt.show()

            except Exception as e:
                txtEntry.insert(tk.END, "Exception: ", ('note'))
                txtEntry.insert(tk.END, e, ('error'))
                txtEntry.insert(tk.END, "  \r\n", ('note'))
                txtEntry.see("end")

    else:
        disableAnimate = True
        plt.close('all')


def csvHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    data = np.array([])
    series = 0
# added to test
#    if csvText.get():
#        loadArray()

    if csvText.get():
        # Ask the user to select a single file name for saving.
        my_filetypes = [("Excel CSV files", ".csv .txt")]
        fileName = ''
        fileName = filedialog.asksaveasfilename(parent=win, initialdir=os.getcwd(
        ), title="Please select a file name for saving:", filetypes=my_filetypes)

        if len(fileName) == 0:
            txtEntry.insert(tk.END, "Filename Error" + "\r\n", ('error'))
            txtEntry.see("end")
            csvText.set(False)
        else:
            try:
                fs = open(fileName, 'a+', newline='')
                fs.write(banner + "\r\n")
                fs.close()
            except:
                txtEntry.insert(
                    tk.END, "File open/create error" + "\r\n", ('error'))
                txtEntry.see("end")
                csvText.set(False)
                fileName = ''
                return

            txtEntry.insert(tk.END, "Writing to file: '" +
                            fileName + "'\r\n", ('note'))
            txtEntry.see("end")
            csvText.set(True)
            plot.set(True)

    else:
        fileName = ''


def connectHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    if connected.get():
        txtEntry.insert(tk.END, "Connecting to serial port " +
                        comPortString.get() + "\r\n", ('note'))
        txtEntry.see("end")

        flags = flags | CONST.CONNECTED

        if (not openSerialPort(comPortString.get(), int(baudRateString.get()))):
            messagebox.showerror(
                "Fatal Error", "Error Opening port " + comPortString.get())
            terminate()
        else:
            serialLoop = True
            thd = threading.Thread(target=serialHandler, daemon=True)
            thd.start()
            entButton.configure(fg='black', bg='green', state=tk.NORMAL)
            plotButton.configure(state=tk.NORMAL)
            csvButton.configure(state=tk.NORMAL)
            plotTypeOption.configure(state=tk.NORMAL)
            cmdEntry.configure(state=tk.NORMAL)
            comPortOption.configure(state=tk.DISABLED)
            baudRateOption.configure(state=tk.DISABLED)
            executeButton.configure(state=tk.DISABLED)
            plot.set(False)
            csvText.set(False)

    else:
        enumerateComPorts()
        terminateSerialHandler()


def terminateSerialHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    flags = flags & ~CONST.CONNECTED
    serialLoop = False
    time.sleep(1)

    if ser.is_open:
        ser.close()

    if thd.is_alive():
        thd.join()

    plot.set(False)
    csvText.set(False)
    plotButton.configure(state=tk.DISABLED)
    csvButton.configure(state=tk.DISABLED)
    plotTypeOption.configure(state=tk.DISABLED)
    entButton.configure(fg='black', bg='red', state=tk.DISABLED)
    cmdEntry.configure(state=tk.DISABLED)
    executeButton.configure(state=tk.DISABLED)
    txtEntry.insert(tk.END, "\r\nDisconnected from serial port\r\n", ('note'))
    txtEntry.see("end")


def entButtonHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    flags = flags | CONST.SEND_COMMAND


def cmdCallback(action, ch, strbefore, strafter, position):
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    return True


def writeCSV(localdata):
   # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    if csvText.get():
        if len(fileName) != 0:
            with open(fileName, 'a+', newline='') as file:
                writer = csv.writer(file)
                writer.writerows(localdata)
            file.close()
        else:
            txtEntry.insert(tk.END, "File Update Error" + "\r\n", ('error'))
            txtEntry.see("end")
            csvText.set(False)


def openSerialPort(port, baud):
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton
    try:
        ser = serial.Serial(port, baud, timeout=1)
        if ser.is_open:
            serialLoop = True
            txtEntry.insert(
                tk.END, "Sucessful connection to " + port + " \r\n", ('note'))
            txtEntry.see("end")
        else:
            ser = False
            serialLoop = False
            txtEntry.insert(
                tk.END, "Error opening serial port " + port + "\r\n", ('error'))
            txtEntry.see("end")
        return serialLoop
    except:
        ser = False
        txtEntry.insert(tk.END, "Serial port " + port +
                        " does not exist or is locked\r\n", ('error'))
        return False


def serialHandler():
    # System and Window ontrol
    global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
    global win, thd, disableAnimate
    # Serial port ontrol
    global ser, serialLoop
    # Data variables
    global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
    # Widgets
    global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
    global comPortOption, baudRateOption, connectButton

    txtEntry.insert(
        tk.END, "Serial Port handler running for "+comPortString.get() + "\r\n", ('note'))
    txtEntry.see("end")

    data = np.array([])
    tempdata = np.array([])
    series = 0
    entries = 0

    while serialLoop:
        if flags & CONST.SEND_COMMAND:
            string = cmd.get() + "\r\n"
            ser.write(string.encode())
            flags = flags & ~CONST.SEND_COMMAND
            cmdEntry.delete(0, 'end')

        while ser.in_waiting > 0:
            try:
                rawline = ser.readline().decode('utf-8')
                rawline = rawline.rstrip('\r')
                rawline = rawline.rstrip('\n')
                if (len(rawline) == 0):
                    continue

                elif rawline[0] == '#' and (plot.get() or csvText.get()):
                    line = rawline.split('#')
                    values = line[1].split(',')
                    if plotTypeString.get() == CONST.TYPE_XY:
                        if len(line[1]) < 3 or len(values) != 2 or not (is_int(values[0]) or is_float(values[0])):
                            txtEntry.insert(
                                tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
                            txtEntry.see("end")
                        else:
                            if is_NumericArray(values):
                                floatvalues = np.array(values).astype(float)
                                if (data.size == 0):
                                    data = np.array(floatvalues)
                                    data = data.reshape(1, 2)
                                else:
                                    data = np.vstack([data, floatvalues])
                                txtEntry.insert(tk.END, "Captured: " +
                                                str(floatvalues[0]) + ", " + str(floatvalues[1]) + "\r\n", ('note'))
                                txtEntry.see("end")
                                entries += 1

                    elif plotTypeString.get() == CONST.TYPE_SXY:
                        if len(line[1]) < 5 or len(values) != 3 or not (is_int(values[0]) or is_float(values[0])):
                            txtEntry.insert(
                                tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
                            txtEntry.see("end")

                        else:
                            if is_NumericArray(values):
                                floatvalues = np.array(values).astype(float)
                                if (tempdata.size == 0):
                                    tempdata = np.array(floatvalues)
                                    tempdata = tempdata.reshape(1, 3)
                                else:
                                    tempdata = np.vstack(
                                        [tempdata, floatvalues])
                                txtEntry.insert(tk.END, "Captured: " +
                                                str(floatvalues[0]) + ", " + str(floatvalues[1]) + ", " + str(floatvalues[2]) + "\r\n", ('note'))
                                txtEntry.see("end")
                                entries += 1

                    elif plotTypeString.get() == CONST.TYPE_Y:
                        if len(line[1]) < 1 or len(values) != 1 or not (is_int(values[0]) or is_float(values[0])):
                            txtEntry.insert(
                                tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
                            txtEntry.see("end")
                        else:
                            if (data.size == 0):
                                data = np.array(float(values[0]))
                                data = data.reshape(1, 1)
                            else:
                                data = np.vstack([data, float(values[0])])

                            txtEntry.insert(tk.END, "Captured: " +
                                            str(values[0]) + "\r\n", ('note'))
                            txtEntry.see("end")
                            entries += 1

                    elif plotTypeString.get() == CONST.TYPE_LIVEXY:
                        if len(line[1]) < 3 or len(values) != 2 or not (is_int(values[0]) or is_float(values[0])):
                            txtEntry.insert(
                                tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
                            txtEntry.see("end")
                        else:
                            if is_NumericArray(values):
                                floatvalues = np.array(values).astype(float)
                                if (data.size == 0):
                                    data = np.array(floatvalues)
                                    data = data.reshape(1, 2)
                                else:
                                    data = np.vstack([data, floatvalues])
                                    txtEntry.insert(
                                        tk.END, "Captured: " + str(floatvalues[0]) + ", " + str(floatvalues[1]) + "\r\n", ('note'))
                                    txtEntry.see("end")
                                series += 1
                                entries += 1
                                if (series > 5):
                                    series = 0
                                    disableAnimate = False

                    elif plotTypeString.get() == CONST.TYPE_LIVEY:
                        if len(line[1]) < 1 or len(values) != 1 or not (is_int(values[0]) or is_float(values[0])):
                            txtEntry.insert(
                                tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
                            txtEntry.see("end")
                        else:
                            if (data.size == 0):
                                data = np.array(float(values[0]))
                                data = data.reshape(1, 1)
                            else:
                                data = np.vstack([data, float(values[0])])
                                txtEntry.insert(tk.END, "Captured: " +
                                                str(values[0]) + "\r\n", ('note'))
                                txtEntry.see("end")
                            series += 1
                            entries += 1
                            if (series > 4):
                                series = 0
                                disableAnimate = False

                elif rawline[0] == '=' and (plot.get() or csvText.get()):
                    if tempdata.size == 0 or tempdata.ndim == 0:
                        txtEntry.insert(
                            tk.END, "Data format error. '=' found without data\r\n", ('error'))
                        txtEntry.see("end")
                        continue

                    elif entries < CONST.MINIMUM_ENTRIES:
                        txtEntry.insert(
                            tk.END, "Data format error. '=' found with insuffient data. Skipping series.\r\n", ('error'))
                        txtEntry.see("end")
                        tempdata = np.array([])
                        entries = 0
                        continue

                    elif plotTypeString.get() == CONST.TYPE_LIVEXY or plotTypeString.get() == CONST.TYPE_LIVEY:
                        txtEntry.insert(
                            tk.END, "Data format error.  '=' found during live plotting\r\n", ('error'))
                        txtEntry.see("end")
                        continue

                    elif plotTypeString.get() != CONST.TYPE_SXY:
                        txtEntry.insert(
                            tk.END, "Data format error. '=' found in data\r\n", ('error'))
                        txtEntry.see("end")
                        continue

                    if series == 0:
                        data = np.array(tempdata)
                    else:
                        datarows, datacols = data.shape
                        temprows, tempcols = tempdata.shape
                        if (temprows == datarows):
                            data = appendArrays(data, tempdata)
                        else:
                            txtEntry.insert(
                                tk.END, "Data format error. Invalid number of entries. Skipping series\r\n", ('error'))
                            txtEntry.see("end")
                            continue

                    tempdata = np.array([])
                    series += 1

                elif rawline[0] == '|' and (plot.get() or csvText.get()):
                    if data.size == 0 or data.ndim == 0:
                        txtEntry.insert(
                            tk.END, "Data format error. '|' found without data\r\n", ('error'))
                        txtEntry.see("end")
                        entries = 0
                        plot.set(False)
                        continue

                    elif entries < CONST.MINIMUM_ENTRIES:
                        txtEntry.insert(
                            tk.END, "Data format error. '|' found with insuffient data\r\n", ('error'))
                        txtEntry.see("end")
                        entries = 0
                        plot.set(False)
                        continue

                    elif plotTypeString.get() == CONST.TYPE_LIVEXY or plotTypeString.get() == CONST.TYPE_LIVEY:
                        txtEntry.insert(
                            tk.END, "Data format error.  '|' found during live plotting\r\n", ('error'))
                        txtEntry.see("end")
                        continue

                    if len(tempdata) > 0 and plotTypeString.get() == CONST.TYPE_SXY:
                        datarows, datacols = data.shape
                        temprows, tempcols = tempdata.shape
                        if (temprows == datarows):
                            data = appendArrays(data, tempdata)
                            series += 1
                        else:
                            txtEntry.insert(
                                tk.END, "Data format error. Invalid number of entries. Skipping series\r\n", ('error'))
                            txtEntry.see("end")
                            entries = 0

                    executeButton.configure(state=tk.NORMAL)
                    txtEntry.insert(tk.END, "\r\nCaptured " + str(len(data)) + " data pairs for " + str(
                        series) + " series.  Press \'GO\' to plot\r\n", ('note'))
                    txtEntry.see("end")
                    entries = 0

                else:
                    txtEntry.insert(tk.END, rawline+"\r\n")
                    txtEntry.see("end")
            except:
                txtEntry.insert(tk.END, ".", ('error'))
                continue


def is_int(n):
    try:
        float_n = float(n)
        int_n = int(float_n)
    except ValueError:
        return False
    else:
        return float_n == int_n


def is_float(n):
    try:
        float_n = float(n)
    except ValueError:
        return False
    else:
        return True


def is_NumericArray(x):
    for el in x:
        if not is_float(el) and not is_int(el):
            return False
    return True


def appendArrays(data, tempdata):
    s = np.array(tempdata[:, 0]).reshape(data.shape[0], 1)
    x = np.array(tempdata[:, 1]).reshape(data.shape[0], 1)
    y = np.array(tempdata[:, 2]).reshape(data.shape[0], 1)
    data = np.hstack((data, s))
    data = np.hstack((data, x))
    data = np.hstack((data, y))
    return data


def round_sig(x, sig=4):
    return round(x, sig-int(math.floor(math.log10(abs(x))))-1)


class _Const(object):
    CONNECTED = int("0x00000001", 0)
    SEND_COMMAND = int("0x00000002", 0)
    PLOT = int("0x00000004", 0)
    SAVE = int("0x00000008", 0)
    ENCODING = 'utf-8'

    SETUP_PLOT = 1
    SETUP_TICKS = 2
    NOTHING = 0
    MINIMUM_ENTRIES = 2

    TYPE_XY = "x,y"
    TYPE_SXY = "s,x,y"
    TYPE_Y = "y"
    TYPE_LIVEXY = "Live x,y"
    TYPE_LIVEY = "Live y"

#################################################################################################

# def loadArray ():
#     global lines

#     lines = []
#     fs = open("sxy.txt")

#     temp = fs.readlines()

#     for el in temp:
#         el = el.rstrip("\r")
#         el = el.rstrip("\n")
#         lines.append(el)

#     fs.close()

#     serialLoop = True
#     thd = threading.Thread(target=Test_serialHandler, daemon=True)
#     thd.start()


# def Test_connectHandler():
#     # System and Window ontrol
#     global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
#     global win, thd, disableAnimate
#     # Serial port ontrol
#     global ser, serialLoop
#     # Data variables
#     global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
#     # Widgets
#     global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
#     global comPortOption, baudRateOption, connectButton

#     if connected.get():
#         flags = flags | CONST.CONNECTED
# #        serialLoop = True
# #        thd = threading.Thread(target=Test_serialHandler, daemon=True)
# #        thd.start()
#         entButton.configure(fg='black', bg='green', state=tk.NORMAL)
#         plotButton.configure(state=tk.NORMAL)
#         csvButton.configure(state=tk.NORMAL)
#         plotTypeOption.configure(state=tk.NORMAL)
#         plotTypeString.set(CONST.TYPE_XY)
#         cmdEntry.configure(state=tk.NORMAL)
#     #    comPortOption.configure(state=tk.DISABLED)
#         baudRateOption.configure(state=tk.DISABLED)
#         executeButton.configure(state=tk.DISABLED)
#     else:
# #        terminateSerialHandler()
#         plot.set(False)
#         csvText.set(False)
#         plotButton.configure(state=tk.DISABLED)
#         csvButton.configure(state=tk.DISABLED)
#         plotTypeOption.configure(state=tk.DISABLED)
#         entButton.configure(fg='black', bg='red', state=tk.DISABLED)
#         cmdEntry.configure(state=tk.DISABLED)
#         executeButton.configure(state=tk.DISABLED)
#         txtEntry.insert(tk.END, "\r\nDisconneted from serial port\r\n", ('note'))


# def Test_serialHandler():
#     # System and Window ontrol
#     global screenWidth, screenHeight, banner, flags, nrows, ncolumns, txtEntryLastRow
#     global win, thd, disableAnimate
#     # Serial port ontrol
#     global ser, serialLoop
#     # Data variables
#     global data, series, cmd, connected, plot, csvText, plotTypeString, baudRateString, comPortString, fileName
#     # Widgets
#     global entButton, executeButton,  cmdEntry, txtEntry, plotButton, csvButton, plotTypeOption
#     global comPortOption, baudRateOption, connectButton

#     txtEntry.insert(
#         tk.END, "Serial Port handler running for "+comPortString.get() + "\r\n", ('note'))
#     txtEntry.see("end")

#     data = np.array([])
#     tempdata = np.array([])
#     series = 0
#     entries = 0

# # added for testing
#     global lines

#     for rawline in lines:
#         try:
#             rawline = ser.readline().decode('utf-8')
#             rawline = rawline.rstrip('\r')
#             rawline = rawline.rstrip('\n')
#             if (len(rawline) == 0):
#                 continue

#             elif rawline[0] == '#' and (plot.get() or csvText.get()):
#                 line = rawline.split('#')
#                 values = line[1].split(',')
#                 if plotTypeString.get() == CONST.TYPE_XY:
#                     if len(line[1]) < 3 or len(values) != 2 or not (is_int(values[0]) or is_float(values[0])):
#                         txtEntry.insert(tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
#                         txtEntry.see("end")
#                     else:
#                         if is_NumericArray (values):
#                             floatvalues = np.array(values).astype(float)
#                             if (data.size == 0):
#                                 data = np.array(floatvalues)
#                                 data = data.reshape(1,2)
#                             else:
#                                 data = np.vstack ([data, floatvalues])
#                             txtEntry.insert(tk.END, "Captured: " +
#                                     str(floatvalues[0]) + ", " + str(floatvalues[1]) + "\r\n", ('note'))
#                             txtEntry.see("end")
#                             entries += 1

#                 elif plotTypeString.get() == CONST.TYPE_SXY:
#                     if len(line[1]) < 5 or len(values) != 3 or not (is_int(values[0]) or is_float(values[0])):
#                         txtEntry.insert(tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
#                         txtEntry.see("end")
#                     else:
#                         if is_NumericArray (values):
#                             floatvalues = np.array(values).astype(float)
#                             if (tempdata.size == 0):
#                                 tempdata = np.array(floatvalues)
#                                 tempdata = tempdata.reshape(1,3)
#                             else:
#                                 tempdata = np.vstack ([tempdata, floatvalues])
#                             txtEntry.insert(tk.END, "Captured: " +
#                                     str(floatvalues[0]) + ", " + str(floatvalues[1]) + ", " + str(floatvalues[2]) + "\r\n", ('note'))
#                             txtEntry.see("end")
#                             entries += 1

#                 elif plotTypeString.get() == CONST.TYPE_Y:
#                     if len(line[1]) < 1 or len(values) != 1 or not (is_int(values[0]) or is_float(values[0])):
#                         txtEntry.insert(tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
#                         txtEntry.see("end")
#                     else:
#                         if (data.size == 0):
#                             data = np.array(float(values[0]))
#                             data = data.reshape(1,1)
#                         else:
#                             data = np.vstack ([data, float(values[0])])

#                         txtEntry.insert(tk.END, "Captured: " +
#                                 str(values[0]) + "\r\n", ('note'))
#                         txtEntry.see("end")
#                         entries += 1

#                 elif plotTypeString.get() == CONST.TYPE_LIVEXY:
#                     if len(line[1]) < 3 or len(values) != 2 or not (is_int(values[0]) or is_float(values[0])):
#                         txtEntry.insert(tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
#                         txtEntry.see("end")
#                     else:
#                         if is_NumericArray (values):
#                             floatvalues = np.array(values).astype(float)
#                             if (data.size == 0):
#                                 data = np.array(floatvalues)
#                                 data = data.reshape(1,2)
#                             else:
#                                 data = np.vstack ([data, floatvalues])
#                                 txtEntry.insert(tk.END, "Captured: " + str(floatvalues[0]) + ", " + str(floatvalues[1]) + "\r\n", ('note'))
#                                 txtEntry.see("end")
#                             series += 1
#                             entries += 1
#                             if (series > 5):
#                                 series = 0
#                                 disableAnimate = False

#                 elif plotTypeString.get() == CONST.TYPE_LIVEY:
#                     if len(line[1]) < 1 or len(values) != 1 or not (is_int(values[0]) or is_float(values[0])):
#                         txtEntry.insert(tk.END, "Data format error. Skipping capturing line: " + line[1] + "\r\n", ('error'))
#                         txtEntry.see("end")
#                     else:
#                         if (data.size == 0):
#                             data = np.array(float(values[0]))
#                             data = data.reshape(1,1)
#                         else:
#                             data = np.vstack ([data, float(values[0])])
#                             txtEntry.insert(tk.END, "Captured: " +
#                                     str(values[0]) + "\r\n", ('note'))
#                             txtEntry.see("end")
#                         series += 1
#                         entries += 1
#                         if (series > 4):
#                             series = 0
#                             disableAnimate = False

#             elif rawline[0] == '=' and (plot.get() or csvText.get()):
#                 if tempdata.size == 0 or tempdata.ndim == 0:
#                     txtEntry.insert(tk.END, "Data format error. '=' found without data\r\n", ('error'))
#                     txtEntry.see("end")
#                     continue

#                 elif entries < CONST.MINIMUM_ENTRIES:
#                     txtEntry.insert(tk.END, "Data format error. '=' found with insuffient data. Skipping series.\r\n", ('error'))
#                     txtEntry.see("end")
#                     tempdata = np.array([])
#                     entries = 0
#                     continue

#                 elif plotTypeString.get() == CONST.TYPE_LIVEXY or plotTypeString.get() == CONST.TYPE_LIVEY:
#                     txtEntry.insert(tk.END, "Data format error.  '=' found during live plotting\r\n", ('error'))
#                     txtEntry.see("end")
#                     continue

#                 elif plotTypeString.get() != CONST.TYPE_SXY:
#                     txtEntry.insert(tk.END, "Data format error. '=' found in data\r\n", ('error'))
#                     txtEntry.see("end")
#                     continue


#                 if series == 0:
#                     data = np.array(tempdata)
#                 else:
#                     datarows, datacols = data.shape
#                     temprows, tempcols = tempdata.shape
#                     if (temprows == datarows):
#                         data = appendArrays (data, tempdata)
#                     else:
#                         txtEntry.insert(tk.END, "Data format error. Invalid number of entries. Skipping series\r\n", ('error'))
#                         txtEntry.see("end")
#                         entries = 0
#                         continue

#                 tempdata = np.array([])
#                 series += 1
#                 entries = 0


#             elif rawline[0] == '|' and (plot.get() or csvText.get()):

#                 if data.size == 0 or data.ndim == 0:
#                     txtEntry.insert(tk.END, "Data format error. '|' found without data\r\n", ('error'))
#                     txtEntry.see("end")
#                     entries = 0
#                     plot.set(False)
#                     continue

#                 elif entries < CONST.MINIMUM_ENTRIES:
#                     txtEntry.insert(tk.END, "Data format error. '|' found with insuffient data\r\n", ('error'))
#                     txtEntry.see("end")
#                     entries = 0
#                     plot.set(False)
#                     continue

#                 elif plotTypeString.get() == CONST.TYPE_LIVEXY or plotTypeString.get() == CONST.TYPE_LIVEY:
#                     txtEntry.insert(tk.END, "Data format error.  '|' found during live plotting\r\n", ('error'))
#                     txtEntry.see("end")
#                     continue

#                 if plotTypeString.get() == CONST.TYPE_SXY:
#                     datarows, datacols = data.shape
#                     temprows, tempcols = tempdata.shape
#                     if (temprows == datarows):
#                         data = appendArrays (data, tempdata)
#                         series += 1
#                     else:
#                         txtEntry.insert(tk.END, "Data format error. Invalid number of entries. Skipping series\r\n", ('error'))
#                         txtEntry.see("end")
#                         entries = 0

#                 executeButton.configure(state=tk.NORMAL)
#                 txtEntry.insert(tk.END, "\r\nCaptured " + str(len(data)) + " data pairs for " + str(series) + " series.  Press \'GO\' to plot\r\n", ('note'))
#                 txtEntry.see("end")
#                 entries = 0

#             else:
#                 txtEntry.insert(tk.END, rawline+"\r\n")
#                 txtEntry.see("end")
#         except:
#             txtEntry.insert(tk.END, ".", ('error'))
#             continue

CONST = _Const()

mainCode()

root.mainloop()
