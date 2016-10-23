# Electromechanical ChessBoard Index.py
# James Fotherby - October 2016
#
# Test whether the serial is working ok

import serial

#----------------------------- Main --------------------------------
ser = serial.Serial('/dev/ttyACM0', 115200)								# Opens a serial port     

while True:
	move = input("Enter move: ")
	ser.write(move.encode())											# Output user's move to Arduino
	

	
	
	
	
	
	
	
	
	
	