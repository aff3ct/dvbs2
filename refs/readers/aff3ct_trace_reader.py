#!/usr/bin/env python3
import re
import numpy as np

# split a string command into a list of string arguments
def splitAsCommand(runCommand):
	argsList = [""]
	idx = 0
	new = 0
	found_dashes = 0

	for s in runCommand:
		if found_dashes == 0:
			if s == ' ':
				if new == 0:
					argsList.append("")
					idx = idx + 1
					new = 1

			elif s == '\"':
				if new == 0:
					argsList.append("")
					idx = idx + 1
					new = 1
				found_dashes = 1

			else:
				argsList[idx] += s
				new = 0

		else: # between dashes
			if s == '\"':
				argsList.append("")
				idx = idx + 1
				found_dashes = 0

			else:
				argsList[idx] += s

	if argsList[idx] == "":
		del argsList[idx]

	return argsList

# read all the lines from the given file and set them in a list of string lines with striped \n \r
def readFileInTable(filename):
	aFile = open(filename, "r")
	lines = []
	for line in aFile:
		line = re.sub('\r','',line.rstrip('\n'))
		if len(line) > 0:
			lines.append(line)
	aFile.close()

	return lines;


class aff3ctTraceReader:
	NoiseLegendsList = { "ebn0" : "Eb/N0", # Signal noise ration view from info bits
	                     "esn0" : "Es/N0", # SNR view from sent symbols
	                     "rop"  : "ROP",   # Received optical power
	                     "ep"   : "EP"     # Event probability
	                   }
	NoiseUnitsList   = { "ebn0" : "dB",
	                     "esn0" : "dB",
	                     "rop"  : "",
	                     "ep"   : ""
	                   }
	BferLegendsList  = {"be_rate" : "BER", # Bit error rate
	                    "fe_rate" : "FER", # Frame error rate
	                    "n_be"    : "BE",  # Number of bit errors
	                    "n_fe"    : "FE",  # Number of frame errors
	                    "n_fra"   : "FRA"  # Number of frame played
	                   }
	MiLegendsList    = {"n_trials" : "TRIALS", # number of frame played
	                    "mi"       : "MI",     # Mutual information average
	                    "mi_min"   : "MIN",    # minimum got MI
	                    "mi_max"   : "MAX"     # maximum got MI
	                   }
	TimeLegendsList  = { "sim_thr" : ["SIM_THR", "SIM_CTHR"], # simulation throughput
	                     "el_time" : "ET/RT"    # elapsed time
	                   }
	TimeUnitsList    = { "sim_thr" : "Mb/s",
	                     "el_time" : "hhmmss"
	                   }

	HeaderEmpty  = "#"
	HeaderLevel1 = "# * "
	HeaderLevel2 = "#    "
	HeaderLevel3 = "#    ** "

	# aff3ctOutput must be a filename string object or
	# the content of a file stocked in a list object with one line of the file per line of the table
	# else raise a ValueError exception
	def __init__(self, aff3ctOutput):

		self.Metadata    = {'command': '', 'title': ''}
		self.SimuHeader  = [] # simulation header that is list of trio (title, value, level index)
		self.Legend      = [] # keys of BferLegendsList or MiLegendsList
		self.NoiseType   = "" # key of NoiseLegendsList
		self.Trace       = {} # all trace informations with key as in Legend array
		                      #  and as value a list to represent the column of data

		self.SimuTitle   = [] # save of the simulation title that is some lines to print software name
		self.LegendTitle = [] # save of the legend table to print it

		if type(aff3ctOutput) is str: # then a filename has been given
			self.Metadata['filename'] = aff3ctOutput
			aff3ctOutput = readFileInTable(aff3ctOutput)
		elif type(aff3ctOutput) is list: # then the file content has been given
			pass
		else:
			raise ValueError("Input is neither a str nor a list object")


		traceVersion = self.__detectTraceVersion(aff3ctOutput)
		if traceVersion == 0:
			self.__reader0(aff3ctOutput)
		elif traceVersion == 1:
			self.__reader1(aff3ctOutput)

		self.__findNoiseType()

	def getSplitCommand(self):
		return splitAsCommand(self.Metadata['command']);

	def getNoise(self):
		return self.getTrace(self.NoiseType)

	def getTrace(self, key):
		if self.legendKeyAvailable(key):
			return self.Trace[key]
		else:
			return []

	def getMetadata(self, key):
		if key in self.Metadata:
			return self.Metadata[key]
		else:
			return ""

	def getSimuHeader(self, key):
		for entry in self.SimuHeader:
			if key in entry[0]:
				return entry[1]

		return ""

	def setSimuHeader(self, key, value, level = 3):
		for entry in self.SimuHeader:
			if key in entry[0]:
				entry[1] = value
				return

		self.SimuHeader.append([key, value, level])

	def getNoiseType(self):
		return self.NoiseType

	def legendKeyAvailable(self, key):
		return key in self.Trace and len(self.Trace[key]) != 0

	def getMetadataAsString(self):
		header = "[metadata]\n"
		for key in self.Metadata:
			header += key + "=" + self.Metadata[key] + "\n"

		header += "\n[trace]\n"

		return header

	def getSimuTitleAsString(self):
		header = ""
		for entry in self.SimuTitle:
			header += entry + "\n"

		return header

	def getLegendTitleAsString(self):
		header = ""
		for entry in self.LegendTitle:
			header += entry + "\n"

		return header

	def getSimuHeaderAsString(self):
		header = ""
		for entry in self.SimuHeader:
			if entry[2] == 1:
				header += self.HeaderLevel1 + entry[0] + " " + entry[1] + "\n"
			elif entry[2] == 2:
				header += self.HeaderLevel2 + entry[0] + " " + entry[1] + "\n"
			elif entry[2] == 3:
				header += self.HeaderLevel3 + entry[0] + " = " + entry[1] + "\n"

		return header + self.HeaderEmpty + "\n"

	def getFullHeaderAsString(self):
		return self.getSimuTitleAsString() + self.getSimuHeaderAsString() + self.getLegendTitleAsString()

	def __checkLegend(self, legendTable, colName):
		for key in legendTable:
			if type(legendTable[key]) is list:
				for i in range(len(legendTable[key])):
					if legendTable[key][i] == colName:
						return key

			elif legendTable[key] == colName:
				return key

		return ""

	def __getMatchingLegend(self, colName):
		key = self.__checkLegend(self.NoiseLegendsList, colName)
		if key != "":
			return key

		key = self.__checkLegend(self.BferLegendsList, colName)
		if key != "":
			return key

		key = self.__checkLegend(self.MiLegendsList, colName)
		if key != "":
			return key

		key = self.__checkLegend(self.TimeLegendsList, colName)
		if key != "":
			return key

		return colName

	def __fillLegend(self, line):
		line = line.replace("#", "")
		line = line.replace("||", "|")
		line = line.split('|')

		for i in range(len(line)):
			line[i] = self.__getMatchingLegend(line[i].strip())

		self.Legend = line

	def __findNoiseType(self):
		# find the type of noise used in this simulation
		for n in self.NoiseLegendsList:
			if n in self.Legend:
				self.NoiseType = n
				break

		noiseParam = self.getSimuHeader("Noise type")

		if self.NoiseType == "esn0" and noiseParam == "EBN0":
			self.NoiseType = "ebn0"

		elif self.NoiseType == "ebn0" and noiseParam == "ESN0":
			self.NoiseType = "esn0"

	def __findLine(self, stringArray, string):
		for i in range(len(stringArray)):
			if string in stringArray[i]:
				return i

		return -1

	def __getVal(self, line):
		line = line.replace("#", "")
		line = line.replace("||", "|")
		line = line.split('|')

		valLine = []
		for i in range(len(line)):
			val = float(0.0)

			try:
				val = float(line[i])

				if "inf" in str(val):
					val = float(0.0)

			except ValueError:
				pass

			valLine.append(val)

		return valLine

	def __detectTraceVersion(self, aff3ctOutput):
		if aff3ctOutput[0].startswith("[metadata]"):
			return 1

		if aff3ctOutput[0].startswith("Run command:"):
			return 0

		return 0

	def __parseHeaderLine(self, line):

		line = line.replace("\n", "")
		if line.startswith(self.HeaderLevel1):
			entry = line[len(self.HeaderLevel1):]
			pos = entry.find(" --")
			entry = [entry[:pos], entry[pos+1:]] # keep line separator "-----------------" in second part
			entry.append(1)
			print("Here I am 1!")
			self.SimuHeader.append(entry)

		elif line.startswith(self.HeaderLevel2) and line[len(self.HeaderLevel2)] != ' ' and line[len(self.HeaderLevel2)] != '*':
			entry = line[len(self.HeaderLevel2):]
			pos = entry.find(" --")
			entry = [entry[:pos], entry[pos+1:]] # keep line separator "-----------------" in second part
			entry.append(2)
			print("Here I am 2!")
			self.SimuHeader.append(entry)

		elif line.startswith(self.HeaderLevel3):
			entry = line[len(self.HeaderLevel3):].split(" = ")
			print("Here I am 3!")
			if len(entry) == 2:
				entry.append(3)
				self.SimuHeader.append(entry)

		elif len(line) > 2:
			if len(self.SimuHeader) == 0: # then it's a simu title
				print("Here I am if!")
				self.SimuTitle.append(line)

			else: # then it's a legend title
				print("Here I am else!")
				self.LegendTitle.append(line)

				if(   line.find(self.BferLegendsList["be_rate"]) != -1
				  and line.find(self.BferLegendsList["fe_rate"]) != -1
				  and line.find(self.BferLegendsList["n_fra"  ]) != -1):
					self.__fillLegend(line)

				elif( line.find(self.MiLegendsList["n_trials"]) != -1
				  and line.find(self.MiLegendsList["mi"      ]) != -1):
					self.__fillLegend(line)
			print(line)


	def __reader1(self, aff3ctOutput):
		startMeta  = False;
		startTrace = False;
		allTrace   = []

		for line in aff3ctOutput:
			if line.startswith("[metadata]"):
				startMeta = True;
				continue

			if line.startswith("[trace]"):
				startTrace = True;
				startMeta = False;
				continue

			if startMeta:
				vals = line.split('=', 1);
				if len(vals) == 2:
					self.Metadata[vals[0]] = vals[1].strip();

			elif startTrace:
				if line.startswith("#") and not line.startswith("# End of the simulation"):
					self.__parseHeaderLine(line)

				elif len(self.Legend) != 0:
					d = self.__getVal(line)
					if len(d) == len(self.Legend):
						allTrace.append(d)


		allTrace = np.array(allTrace).transpose()


		# fill trace
		for i in range(len(self.Legend)):
			if len(allTrace) > i:
				self.Trace[self.Legend[i]] = allTrace[i]
			else:
				self.Trace[self.Legend[i]] = []

	def __reader0(self, aff3ctOutput):
		startMeta  = False;
		startTrace = False;
		allTrace   = []

		for line in aff3ctOutput:
			if line.startswith("#") and not line.startswith("# End of the simulation"):
				self.__parseHeaderLine(line)

			elif len(self.Legend) != 0:
				d = self.__getVal(line)
				if len(d) == len(self.Legend):
					allTrace.append(d)


		allTrace = np.array(allTrace).transpose()


		# get the command to reproduce this trace
		idx = self.__findLine(aff3ctOutput, "Run command:")
		if idx != -1 and len(aff3ctOutput) >= idx +1:
			self.Metadata['command'] = str(aff3ctOutput[idx +1].strip())

		# get the curve name (if there is one)
		idx = self.__findLine(aff3ctOutput, "Curve name:")
		if idx != -1 and len(aff3ctOutput) >= idx +1:
			self.Metadata['title'] = str(aff3ctOutput[idx +1].strip())


		# fill trace
		for i in range(len(self.Legend)):
			if len(allTrace) > i:
				self.Trace[self.Legend[i]] = allTrace[i]
			else:
				self.Trace[self.Legend[i]] = []
