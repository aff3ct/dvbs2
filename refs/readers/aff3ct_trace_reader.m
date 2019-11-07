classdef aff3ct_trace_reader < handle
	%AFF3CT_TRACE_READER Summary of this class goes here
	%   Detailed explanation goes here

	properties (Access = public, Constant)
		NoiseLegendsList = containers.Map( ...
							{'ebn0', ... % Signal noise ration view from info bits
							 'esn0', ... % SNR view from sent symbols
							 'rop',  ... % Received optical power
							 'ep'},  ... % Event probability
							{'Eb/N0', 'Es/N0', 'ROP', 'EP'});

		NoiseUnitsList = containers.Map( ...
							{'ebn0', ... % Signal noise ration view from info bits
							 'esn0', ... % SNR view from sent symbols
							 'rop',  ... % Received optical power
							 'ep'},  ... % Event probability
							{'dB', 'dB', '', ''});

		BferLegendsList = containers.Map( ...
							{'be_rate', ... % Bit error rate
							 'fe_rate', ... % Frame error rate
							 'n_be',	... % Number of bit errors
							 'n_fe',	... % Number of frame errors
							 'n_fra'},  ... % Number of frame played
							{'BER', 'FER', 'BE', 'FE', 'FRA'});

		MiLegendsList = containers.Map( ...
							{'n_trials', ... % number of frame played
							 'mi',	   ... % Mutual information average
							 'mi_min',   ... % minimum got MI
							 'mi_max'},  ... % maximum got MI
							{'TRIALS', 'MI', 'MIN', 'MAX'});

		TimeLegendsList = containers.Map( ...
							{'sim_thr',   ... % simulation throughput
							 'el_time'},  ... % elapsed time
							{['SIM_THR', 'SIM_CTHR'], 'ET'});

		TimeUnitsList = containers.Map( ...
							{'sim_thr',   ... % simulation throughput
							 'el_time'},  ... % elapsed time
							{'Mb/s', 'hhmmss'});

		HeaderEmpty  = "#";
		HeaderLevel1 = "# * ";
		HeaderLevel2 = "#    ";
		HeaderLevel3 = "#    ** ";

	end

	properties (Access = private)
		Metadata;   % The Metadata map
		SimuHeader; % simulation header that is list of trio (title, value, level index)
		Legend;     % keys of BferLegendsList or MiLegendsList
		NoiseType;  % key of NoiseLegendsList
		Trace;      % all trace informations with key as in Legend array
		                      %  and as value a list to represent the column of data

		SimuTitle;   % save of the simulation title that is some lines to print software name
		LegendTitle; % save of the legend table to print it
	end

	methods (Access = public)
		function self = aff3ct_trace_reader(aff3ctOutput)
			self.Metadata    = containers.Map({'command', 'title'}, {"", ""});
			self.SimuHeader  = {}; % simulation header that is list of trio (title, value, level index)
			self.Legend      = {}; % keys of BferLegendsList or MiLegendsList
			self.NoiseType   = ''; % key of NoiseLegendsList
			self.Trace       = containers.Map(); % all trace informations with key as in Legend array
			                      %  and as value a list to represent the column of data

			self.SimuTitle   = {}; % save of the simulation title that is some lines to print software name
			self.LegendTitle = {}; % save of the legend table to print it

			narg = nargin;
			if narg == 0
			    % aff3ctOutput = "BCH/AWGN/BPSK/ALGEBRAIC/BCH_N1023_K1003_algebraic_T2.txt";
			    narg = 1;
			    aff3ctOutput = "BCH/AWGN/BPSK/ALGEBRAIC/BCH_N1023_K698_algebraic_T35.txt";
			end

			if narg > 0
				if class(aff3ctOutput) == "char"
					aff3ctOutput = string(aff3ctOutput);
				end

				if class(aff3ctOutput) == "string"
					sizein = size(aff3ctOutput);
					if sizein == [1,1] % then a filename has been given
						self.Metadata('filename') = aff3ctOutput;
						aff3ctOutput = self.readFileInTable(aff3ctOutput);

					elseif sizein(1) ~= 1 && sizein(2) ~= 1
						throw(MException('aff3ct_trace_reader:invalidArgument', ...
						                 'Input is neither a filename nor the content of a file given in' ...
						                 + ' an array of strings (1 line per array element).'));
					end
				else
					throw(MException('aff3ct_trace_reader:invalidArgument', ...
					                 'Input is not a string nor an array of strings.'));
				end

				traceVersion = self.detectTraceVersion(aff3ctOutput);
				if traceVersion == 0
					self.reader0(aff3ctOutput)
				elseif traceVersion == 1
					self.reader1(aff3ctOutput)
				end

				self.findNoiseType();

			else
				throw(MException('aff3ct_trace_reader:invalidArgument', 'Must have an argument.'));
			end
		end

		function command = getSplitCommand(self)
			command = self.splitAsCommand(self.Metadata('command'));
		end

		function noiseTrace = getNoise(self)
			noiseTrace = self.getTrace(self.NoiseType);
		end

		function trace = getTrace(self, key)
			if self.legendKeyAvailable(key)
				trace = self.Trace(key);
			else
				trace = [];
			end
		end

		function val = getMetadata(self, key)
			if self.Metadata.isKey(key)
				val = self.Metadata(key);
			else
				val = "";
			end
		end

		function val = getSimuHeader(self, key)
			for i = 1:numel(self.SimuHeader)
				head = self.SimuHeader(i);
                head = head{1};
				if strfind(string(head{1}), key) > 0
					val = head{2};
					return;
				end
			end
			val = "";
		end

		function setSimuHeader(self, key, value, level)
			if ~exist('level', 'var')
				level = 3;
			end

			for i = 1:numel(self.SimuHeader)
				head = self.SimuHeader(i);
                head = head{1};
				if strfind(head{1}, key) > 0
					head{2} = value;
                    self.SimuHeader(i) = head;
					return;
				end
			end

			self.SimuHeader{end+1} = {key, value, level};
		end

		function nt = getNoiseType(self)
			nt = self.NoiseType;
		end

		function available = legendKeyAvailable(self, key)
			available = self.Trace.isKey(key);
		end

		function header = getMetadataAsString(self)
			header = "[metadata]\n";
			keys = self.Metadata.keys();
			for k = 1:numel(keys)
				header = header + string(keys(k)) + "=" + self.Metadata(key) + "\n";
			end

			header = header + "\n[trace]\n";
		end

		function header = getSimuTitleAsString(self)
			header = "";
			for k = 1:numel(self.SimuTitle)
				header = header + string(self.SimuTitle(k)) + "\n";
			end
		end

		function header = getLegendTitleAsString(self)
			header = "";
			for k = 1:numel(self.LegendTitle)
				header = header + string(self.LegendTitle(k)) + "\n";
			end
		end

		function header = getSimuHeaderAsString(self)
			header = "";
			for k = 1:numel(self.SimuHeader)
				entry = self.SimuHeader(k);
				if entry(2) == 1
					header = header + self.HeaderLevel1 + string(entry(0)) + " " + string(entry(1)) + "\n";
				elseif entry(2) == 2
					header = header + self.HeaderLevel2 + string(entry(0)) + " " + string(entry(1)) + "\n";
				elseif entry(2) == 3
					header = header + self.HeaderLevel3 + string(entry(0)) + " = " + string(entry(1)) + "\n";
				end
			end

			header = header + self.HeaderEmpty + "\n";
		end

		function header = getFullHeaderAsString(self)
			header = self.getSimuTitleAsString() + self.getSimuHeaderAsString() + self.getLegendTitleAsString();
		end
	end

	methods (Access = public, Static)
		% read all the lines from the given file and set them in a list of string lines with striped \n \r
		function lines = readFileInTable(filename)
			fid = fopen(filename, 'r');
			tline = fgetl(fid);
			lines = {};

			while ischar(tline)
				lines{end+1} = string(tline);
				tline = fgetl(fid);
			end

			fclose(fid);
		end

		function version = detectTraceVersion(aff3ctOutput)
			if aff3ctOutput{1}.startsWith("[metadata]")
				version = 1;
			elseif aff3ctOutput{1}.startsWith("Run command")
				version = 0;
			else
				version = 0;
			end
		end

		function command = splitAsCommand(runCommand)
			argsList = {""};
			new = false;
			found_dashes = false;

			for i = 1:length(runCommand)
				s = runCommand(i);
				if ~found_dashes
					if s == ' '
						if ~new
							argsList{end+1} = "";
							new = true;
						end

					elseif s == '"'
						if ~new
							argsList{end+1} = "";
							new = true;
						end
						found_dashes = true;

					else
						argsList{end} = argsList{end} + s;
						new = false;
					end

				else % between dashes
					if s == '"'
                        argsList{end+1} = "";
						found_dashes = false;
					else
						argsList{end} = argsList{end} + s;
					end
				end
			end

			if argsList{end} == ""
				command = {argsList{1:end-1}};
			else
				command = argsList;
			end
		end
	end

	methods (Access = private, Static)
		function valLine = getVal(line)
			line = line.replace('#', '');
			line = line.replace('||', '|');
			line = strtrim(strsplit(line, '|'));

			valLine = [];
			for i = 1:numel(line)
				val = str2num(char(line(i)));

				if isempty(val) || isinf(val) || isnan(val)
					val = 0.0;
				end

				valLine = [valLine val];
			end
		end

		function idx = findLine(stringArray, string)
			for idx = 1:numel(stringArray)
				if ~isempty(strfind(stringArray{idx}, string))
					return
				end
			end

			idx = -1;
		end

		function key = checkLegend(legendTable, colName)
			keys = legendTable.keys  ;
			vals = legendTable.values;
			for k = 1:size(legendTable,1)
				key = keys{k};
				val = vals(k); % val needs to be treated as a cell
				for i = 1:numel(val)
					if val{i} == colName
						return
					end
				end
			end
			key = "";
		end
	end

	methods (Access = private)
		function key = getMatchingLegend(self, colName)
			key = self.checkLegend(self.NoiseLegendsList, colName);
			if key ~= ""
				return
			end

			key = self.checkLegend(self.BferLegendsList, colName);
			if key ~= ""
				return
			end

			key = self.checkLegend(self.MiLegendsList, colName);
			if key ~= ""
				return
			end

			key = self.checkLegend(self.TimeLegendsList, colName);
			if key ~= ""
				return
			end

			key = colName;
		end

		function fillLegend(self, line)
			line = line.replace('#', '');
			line = line.replace('||', '|');
			line = strtrim(strsplit(line, '|'));

			for i = 1:numel(line)
				line(i) = self.getMatchingLegend(line(i));
			end

			self.Legend = cellstr(line);
		end

		function findNoiseType(self)
			% find the type of noise used in this simulation
			keys = self.NoiseLegendsList.keys();
			for n = 1:size(self.NoiseLegendsList,1)
				if ~isempty(find(strcmp(self.Legend, keys{n})))
					self.NoiseType = keys{n};
					break
				end
			end

			noiseParam = self.getSimuHeader("Noise type");

			if self.NoiseType == "esn0" && noiseParam == "EBN0"
				self.NoiseType = "ebn0";

			elseif self.NoiseType == "ebn0" && noiseParam == "ESN0"
				self.NoiseType = "esn0";
			end
		end

		function parseHeaderLine(self, line)
			cline = char(line);
			if line.startsWith(self.HeaderLevel1)
				entry = extractAfter(line, strlength(self.HeaderLevel1));
				pos   = strfind(entry, ' --');
				pos   = pos(1);
				entry = {extractBefore(entry,pos), extractAfter(entry,pos+1), 1}; % keep line separator '-----------------' in second part
				self.SimuHeader{end+1} = entry;

			elseif line.startsWith(self.HeaderLevel2) && cline(strlength(self.HeaderLevel2)) ~= ' ' && cline(strlength(self.HeaderLevel2)) ~= '*'
				entry = extractAfter(line, strlength(self.HeaderLevel2));
				pos   = strfind(entry, ' --');
				pos   = pos(1);
				entry = {extractBefore(entry,pos), extractAfter(entry,pos+1), 2}; % keep line separator '-----------------' in second part
				self.SimuHeader{end+1} = entry;

			elseif line.startsWith(self.HeaderLevel3)
				entry = extractAfter(line, strlength(self.HeaderLevel3));
				pos   = strfind(entry, ' = ');
				if ~isempty(pos)
					pos   = pos(1);
					entry = {extractBefore(entry,pos), extractAfter(entry,pos+2), 3};
					self.SimuHeader{end+1} = entry;
				end

			elseif strlength(line) > 2
				if numel(self.SimuHeader) == 0 % then it's a simu title
					self.SimuTitle{end+1} = line;

				else % then its a legend title
					self.LegendTitle{end+1} = line;

					if(  ~isempty(strfind(line, self.BferLegendsList('be_rate'))) ...
					  && ~isempty(strfind(line, self.BferLegendsList('fe_rate'))) ...
					  && ~isempty(strfind(line, self.BferLegendsList('n_fra'  ))))
						self.fillLegend(line)

					elseif(~isempty(strfind(line, self.MiLegendsList('n_trials'))) ...
					    && ~isempty(strfind(line, self.MiLegendsList('mi'      ))))
						self.fillLegend(line)
					end
				end
			end
		end

		function reader1(self, aff3ctOutput)
			startMeta  = false;
			startTrace = false;
			allTrace   = [];

			for i = 1:numel(aff3ctOutput)
				line = aff3ctOutput{i};
				if line.startsWith('[metadata]')
					startMeta = true;
					continue
				end

				if line.startsWith('[trace]')
					startTrace = true;
					startMeta = false;
					continue
				end

				if startMeta
					vals = strsplit(line, '=');
					if numel(vals) == 2
						self.Metadata(char(vals{1})) = strtrim(vals{2});
					end

				elseif startTrace
					if line.startsWith('#') && ~line.startsWith('# End of the simulation')
						self.parseHeaderLine(line);

					elseif numel(self.Legend) ~= 0
						d = self.getVal(line);
						if numel(d) == numel(self.Legend)
							allTrace = [allTrace; d];
						end
					end
				end
			end

			allTrace = allTrace';


			% fill trace
			for i = 1:numel(self.Legend)
				if size(allTrace,1) > i
					self.Trace(char(self.Legend(i))) = allTrace(i,:);
				else
					self.Trace(char(self.Legend(i))) = [];
				end
			end
		end

		function reader0(self, aff3ctOutput)
			startMeta  = false;
			startTrace = false;
			allTrace   = [];

			for i = 1:numel(aff3ctOutput)
				line = aff3ctOutput{i};
				if line.startsWith('#') && ~line.startsWith('# End of the simulation')
					self.parseHeaderLine(line);

				elseif numel(self.Legend) ~= 0
					d = self.getVal(line);
					if numel(d) == numel(self.Legend)
						allTrace = [allTrace; d];
					end
				end
			end

			allTrace = allTrace';


			% get the command to reproduce this trace
			idx = self.findLine(aff3ctOutput, "Run command");
			if idx ~= -1 && numel(aff3ctOutput) >= (idx +1)
				self.Metadata('command') = strtrim(string(aff3ctOutput{idx +1}));
			end

			% get the curve name (if there is one)
			idx = self.findLine(aff3ctOutput, "Curve name");
			if idx ~= -1 && numel(aff3ctOutput) >= (idx +1)
				self.Metadata('title') = strtrim(string(aff3ctOutput{idx +1}));
			end

			% fill trace
			for i = 1:numel(self.Legend)
				if size(allTrace,1) > i
					self.Trace(char(self.Legend(i))) = allTrace(i,:);
				else
					self.Trace(char(self.Legend(i))) = [];
				end
			end
		end
	end
end

