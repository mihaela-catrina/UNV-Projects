#!/usr/bin/python3
from sys import argv as argv
import io
import datetime  
import re  
import operator


def diff_dates(a,b):
	FMT = '%Y-%m-%dT%H:%M'
	tdelta = datetime.datetime.strptime(b,FMT)-datetime.datetime.strptime(a,FMT)
	return tdelta.days*24*60+tdelta.seconds/60


def key1(a):
	return a.endpoint

def key2(a):
	return (a.date, a.endpoint)
	
class elem:
	
	def __init__(self, date, endpoint):
		self.date = date
		self.endpoint = endpoint
		self.val = []
		
	def sum(self,value):
		self.val.append(int(value))

		
intv = 1			
start = 'Nope'		
end = 'Nope'		
succes = 'Nope'		

lines = []			

length = len(argv)

for j in range(length):
	if argv[j] == '--interval':
		intv = int(argv[j+1])
	if argv[j] == '--start':
		start = argv[j+1]
	if argv[j] == '--end':
		end = argv[j+1]
	if argv[j] == '--success':
		succes = argv[j+1]


if succes != 'Nope':
					
	succes = succes.split(',')
	for i in range(len(succes)):
		succes[i] = list(succes[i])

	i = 0
	while i < len(succes):
		temp = []
		j = 0
		tr = False
		for j in range(len(succes[i])):
			true = succes[i][j].isalpha()
			if true == True:
				poz = j
				if poz == 1: 
					tr = succes[i][poz+1].isalpha()
				if tr == True:
					succes[i][poz] = '0'
					succes[i][poz+1] = '0'
					zero = ''.join(succes[i])
					succes[i][poz] = '9'
					succes[i][poz+1] = '9'
					nine = ''.join(succes[i])
					zero = int(zero)
					nine = int(nine)
					for num in range(zero, nine+1):
						aux = str(num)
						temp.append(aux)		
				else:
					succes[i][poz] = '0'
					zero = ''.join(succes[i])
					succes[i][poz] = '9'
					nine = ''.join(succes[i])
					zero = int(zero)
					nine = int(nine)
					if poz == 2:
						for num in range(zero, nine+1):
							aux = str(num)
							temp.append(aux)
					if poz == 1:
						for num in range(zero, nine+1, 10):
							aux = str(num)
							temp.append(aux)
				
				break

		if  true == True:
			del succes[i]
			succes= succes + temp
		else:
			succes[i] = ''.join(succes[i])
			i += 1
	for i in range(len(succes)):
		succes[i] = int(succes[i])
	succes = sorted(succes)

myfile = open(argv[1], 'r')			
modified_line = []					
diff_start = 0						
diff_end = 0						

if succes == 'Nope':
	with open(argv[1], 'r') as myfile:
		for line in myfile:				
			regex = '([(\d\.)]+) - - \[(.*?) +.*\] "GET (.*?.html).*" (\d+) (\d+) "-" "(.*?)"'
			match_line = re.match(regex,line)
										
			if match_line:
										
				dt = datetime.datetime.strptime(match_line.group(2), '%d/%b/%Y:%H:%M:%S')
				date = dt.strftime('%Y-%m-%dT%H:%M')
	
			if start != 'Nope':			
				diff_start = diff_dates(start, date)
			if end != 'Nope':			
				diff_end = diff_dates(date, end)
										
			if diff_start >= 0 and diff_end >= 0:
				modified_line = elem(date, match_line.group(3))
				x = int(match_line.group(4))
				if x < 200 or x >=300:
					x = 0
				else:
					x = 100
				modified_line.sum(x)
				lines.append(modified_line)
		

else:
	with open(argv[1], 'r') as myfile:
		for line in myfile:
			regex = '([(\d\.)]+) - - \[(.*?) +.*\] "GET (.*?.html).*" (\d+) (\d+) "-" "(.*?)"'
			match_line = re.match(regex,line)
			if match_line:
	
				dt = datetime.datetime.strptime(match_line.group(2), '%d/%b/%Y:%H:%M:%S')
				date = dt.strftime('%Y-%m-%dT%H:%M')
		
			if start != 'Nope':
				diff_start = diff_dates(start, date)
			if end != 'Nope':
				diff_end = diff_dates(date, end)
			if diff_start >= 0 and diff_end >= 0:
				modified_line = elem(date, match_line.group(3))
				x = int(match_line.group(4))
				if x in succes:
					x = 100
				else:
					x = 0
				modified_line.sum(x)	
				lines.append(modified_line)
	


lines = sorted(lines, key=key1)		

k = 1
lg = len(lines)
lines[lg-1].val.append(1)

while k < len(lines):
	nr = 1							
	while k < len(lines) and lines[k].endpoint == lines[k-1].endpoint:
		dif = diff_dates(lines[k-1].date,lines[k].date)
		if dif < intv:				
			lines[k-1].val.append(lines[k].val[0])
			del lines[k]			
			nr += 1				
		else:
			break

	lines[k-1].val.append(nr)		
	k += 1
	
									
for i in range(len(lines)):
	m = len(lines[i].val)
	temp = lines[i].val[0]
	for j in range(m-1): 
		lines[i].val[0] += lines[i].val[j]
	lines[i].val[0] = "%0.2f"%((lines[i].val[0]-temp) / float(lines[i].val[m-1]))


								
lines = sorted(lines, key=key2)
	

for i in range(len(lines)):
	print( lines[i].date, intv, lines[i].endpoint, lines[i].val[0] )


