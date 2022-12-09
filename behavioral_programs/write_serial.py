
import sys, os, threading, serial, time
import csv
     
# serial library is installed with pip install pyserial

def monitor():
   ser = serial.Serial(COMPORT, BAUDRATE, timeout=1)

   time.sleep(1)

  # send parameters
   ser.write(b'1')

   running = 1

   while (running == 1):
       line = ser.readline()
       line = line[0:len(line)-2].decode("utf-8")

       if (line != ""):

          # print line for live display
          print(line)

          # append file
          with open(fn,"a") as f:
              writer = csv.writer(f,delimiter=",")
              writer.writerow([line])

          # stop connection following stop command
          if (int(line.split(" ")[0]) == 0):
            print("Stop Monitoring")
            ser.close()
            running = 0



   

""" -------------------------------------------
MAIN APPLICATION 
"""  

print ("Start Serial Monitor")

COMPORT = "COM11";
BAUDRATE = 115200;

date = '2021_06_23'

#subject = 'aam02'
subject = 'aam05'
#subject = 'aam08'

#subject = 'test'

fn = date + "_" + subject + '.csv'


monitor()
