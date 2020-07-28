function [] = vitalSigns_demo_gui(comportSnum, comportCliNum, cliCfgFileName, loadCfg, app)
%*
% * % * Copyright (C) {2017} Texas Instruments Incorporated - http://www.ti.com/ 
% * ALL RIGHTS RESERVED 
% * 
% * Reads the data output sent out by the Radar EVM 
% * from the UART and plots the Breathing waveform, 
% * Heart-rate waveform, breathing-rate and heart-rate 
% * 
% */

fprintf('Starting UI for vital Signs Demo ....\n'); 
debug = 0;
if (nargin < 4)
    fprintf('!!ERROR:Missing arguments!!\n');
    fprintf('Specify arguments in this order: <comportSnum> <range_depth> <range_width> <comportCliNum> <cliCfgFileName> <loadCfg> \n');
    fprintf('comportSnum             : comport on which visualization data is being sent \n');
    fprintf('comportCliNum           : comport over which the cli configuration is sent to AR14xx \n');
    fprintf('cliCfgFileName          : Input cli configuration file \n');
    fprintf('loadCfg                 : loadCfg=1: cli configuration is sent to AR14xx \n');
    fprintf('                          loadCfg=0: it is assumed that configuration is already sent to AR14xx\n');
    fprintf('                                     and it is already running, the configuration is not sent to AR14xx, \n');
    fprintf('                                     but it will keep receiving and displaying incomming data. \n');
    return;
end
if(ischar(comportSnum))
    comportSnum=str2num(comportSnum);   
end

if ischar(loadCfg)
    loadCfg = str2num(loadCfg);
end

%%% Global Variables %%%%%%%%%%%%%%%%%%%%%%%%%%%
global TOTAL_PAYLOAD_SIZE_BYTES;
global LENGTH_MAGIC_WORD_BYTES;    % Length of Magic Word appended to the UART packet from the EVM
global LENGTH_DEBUG_DATA_OUT_BYTES;    % VitalSignsDemo_OutputStats size
global LENGTH_HEADER_BYTES;
global LENGTH_TLV_MESSAGE_HEADER_BYTES;
global MMWDEMO_OUTPUT_MSG_SEGMENT_LEN; % The data sent out through the UART has Extra Padding to make it a
                                       %  multiple of MMWDEMO_OUTPUT_MSG_SEGMENT_LEN
global bytevec;
global framecou;
global framecountDummy;
global Params;

LENGTH_MAGIC_WORD_BYTES   = 8;
LENGTH_HEADER_BYTES       = 40;   % Header + Magic Word
MMWDEMO_OUTPUT_MSG_SEGMENT_LEN = 32;
LENGTH_DEBUG_DATA_OUT_BYTES =    128;   % VitalSignsDemo_OutputStats size
LENGTH_TLV_MESSAGE_HEADER_BYTES = 8;

framecountDummy = 0;
bytevec = [];
framecou = 0;

PLOT_DISPLAY_LENGTH = 128;  % The number of points to display in the GUI waveforms
valuesUpdateCount = 10;     % Updates the vital signs estimates every nth Count
INITIALIZED = 0;
LENGTH_OFFSET_BYTES   = LENGTH_HEADER_BYTES  - LENGTH_MAGIC_WORD_BYTES + LENGTH_TLV_MESSAGE_HEADER_BYTES

INDEX_GLOBAL_COUNT                      = 21;
INDEX_RANGE_BIN_PHASE                   = 1;
INDEX_RANGE_BIN_VALUE                   = 2;
INDEX_PHASE                             = 5;
INDEX_BREATHING_WAVEFORM                = 6;
INDEX_HEART_WAVEFORM                    = 7;
INDEX_HEART_RATE_EST_FFT                = 8;
INDEX_HEART_INDICES_1                   = 9;
INDEX_HEART_INDICES_2                   = 10;
INDEX_HEART_RATE_EST_PEAK               = 11;
INDEX_BREATHING_RATE_FFT                = 12;
INDEX_HEART_INDICES_3                   = 13;
INDEX_BREATHING_RATE_PEAK               = 14;
INDEX_HEART_INDICES_4                   = 15;
INDEX_HEART_INDICES_5                   = 16;
INDEX_HEART_INDICES_6                   = 17;
INDEX_HEART_INDICES_7                   = 18;
INDEX_HEART_INDICES_8                   = 19;
INDEX_ENERGYWFM_BREATH                  = 20;
INDEX_ENERGYWFM_HEART                   = 21;
INDEX_MOTION_DETECTION                  = 22;
INDEX_RANGE_PROFILE_START               = 35%(LENGTH_DEBUG_DATA_OUT_BYTES+LENGTH_TLV_MESSAGE_HEADER_BYTES)/4  + 1

OFFSET = LENGTH_OFFSET_BYTES + LENGTH_TLV_MESSAGE_HEADER_BYTES;
INDEX_IN_RANGE_BIN_INDEX  = (OFFSET + 3: OFFSET + 4);

INDEX_IN_DATA_RANGE_BIN_VALUE              = TRANSLATE_INDEX(OFFSET ,INDEX_RANGE_BIN_VALUE);
INDEX_IN_DATA_PHASE                        = TRANSLATE_INDEX(OFFSET, INDEX_PHASE);
INDEX_IN_DATA_BREATHING_WAVEFORM           = TRANSLATE_INDEX(OFFSET, INDEX_BREATHING_WAVEFORM);
INDEX_IN_DATA_HEART_WAVEFORM               = TRANSLATE_INDEX(OFFSET, INDEX_HEART_WAVEFORM);
INDEX_IN_DATA_BREATHING_RATE_FFT           = TRANSLATE_INDEX(OFFSET, INDEX_BREATHING_RATE_FFT);
INDEX_IN_DATA_HEART_RATE_EST_FFT           = TRANSLATE_INDEX(OFFSET, INDEX_HEART_RATE_EST_FFT);
INDEX_IN_DATA_HEART_INDICES_1              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_1);
INDEX_IN_DATA_HEART_INDICES_2              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_2);
INDEX_IN_DATA_HEART_INDICES_3              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_3);
INDEX_IN_DATA_HEART_INDICES_4              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_4);
INDEX_IN_DATA_HEART_INDICES_5              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_5);
INDEX_IN_DATA_HEART_INDICES_6              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_6);
INDEX_IN_DATA_HEART_INDICES_7              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_7);
INDEX_IN_DATA_HEART_INDICES_8              = TRANSLATE_INDEX(OFFSET, INDEX_HEART_INDICES_8);
INDEX_IN_DATA_BREATHING_RATE_PEAK          = TRANSLATE_INDEX(OFFSET, INDEX_BREATHING_RATE_PEAK);
INDEX_IN_DATA_HEART_RATE_EST_PEAK          = TRANSLATE_INDEX(OFFSET, INDEX_HEART_RATE_EST_PEAK);
INDEX_IN_DATA_ENERGYWFM_BREATH             = TRANSLATE_INDEX(OFFSET, INDEX_ENERGYWFM_BREATH);
INDEX_IN_DATA_ENERGYWFM_HEART              = TRANSLATE_INDEX(OFFSET, INDEX_ENERGYWFM_HEART);
INDEX_IN_DATA_MOTION_DETECTION_FLAG        = TRANSLATE_INDEX(OFFSET, INDEX_MOTION_DETECTION);
INDEX_IN_DATA_RANGE_PROFILE_START          = TRANSLATE_INDEX(OFFSET, INDEX_RANGE_PROFILE_START);
 
LENGTH_BUFFER = 40;
heartRateEstDisplay_CircBuffer = 48*ones(1,LENGTH_BUFFER);

%Read Configuration file
cliCfgFileId = fopen(cliCfgFileName, 'r');
if cliCfgFileId == -1
    fprintf('File %s not found!\n', cliCfgFileName);
    return
else
    fprintf('Opening configuration file %s ...\n', cliCfgFileName);
end
cliCfg=[];
tline = fgetl(cliCfgFileId);
k=1;
while ischar(tline)
    cliCfg{k} = tline;
    tline = fgetl(cliCfgFileId);
    k = k + 1;
end
fclose(cliCfgFileId);

%Parse CLI parameters
Params = parseCfg(cliCfg);

%Configure monitoring UART port 
sphandle = configureSport(comportSnum);



% Send Configuration Parameters to AR14xx

% Open CLI port
    if ischar(comportCliNum)
        comportCliNum=str2num(comportCliNum);
    end
    spCliHandle = configureCliPort(comportCliNum);

    warning off; %MATLAB:serial:fread:unsuccessfulRead
    timeOut = get(spCliHandle,'Timeout');
    set(spCliHandle,'Timeout',1);
    
if loadCfg == 1
    tStart = tic;
    
    while 1
        fprintf(spCliHandle, ''); heartWfmTemp=fread(spCliHandle,100);
        heartWfmTemp = strrep(strrep(heartWfmTemp,char(10),''),char(13),'');
        if ~isempty(heartWfmTemp)
                break;
        end
        pause(0.1);
        toc(tStart);
    end
    set(spCliHandle,'Timeout', timeOut);
    warning on;    
    
 %Send CLI configuration to AR14xx
    fprintf('Sending configuration to AR14xx %s ...\n', cliCfgFileName);
    for k=1:length(cliCfg)
        fprintf(spCliHandle, cliCfg{k});
        fprintf('%s\n', cliCfg{k});
        pause(.2);
    end
else
    if Params.frameCfg.numFrames == 1
    %Single frame mode, send Frame Start
        fprintf(spCliHandle, 'frameStart');
    end
end

tStart = tic;
tIdleStart = tic;
timeout_ctr = 0;
bytevec_cp = zeros(10*TOTAL_PAYLOAD_SIZE_BYTES,1);
bytevec_cp_len = 0;
startFramecou = 0;
endFramecou = 0;

file_name = 'DataOutput_XWR14xx';
fid_write_bin = fopen(strcat(file_name, '.bin'),'wb');  % By Default open the file 

outBreathPlot = nan(1,PLOT_DISPLAY_LENGTH);outBreathPlot(1) = 0;
outHeartPlot  = nan(1,PLOT_DISPLAY_LENGTH);outHeartPlot(1) = 0;
outPhasePlot  = nan(1,PLOT_DISPLAY_LENGTH);outPhasePlot(1) = 0;
EXIT_KEY_PRESSED = 0;
RESET_KEY_PRESSED = 0;

outSumEnergyBreathWfm_thresh = 3;    % Threshold on the Breathing Waveform 
outSumEnergyHeartWfm_thresh =  0.05; % Threshold on the HeartRate Waveform 
thresh_HeartCM = 0.4;                % Threshold on the heartrate CM
thresh_diffEst = 15;                 % Threshold on the difference between heartRate and peakbased estimate   
rangeBinValueThresh = 250;

rangeStart_meter = Params.vitalSignsParams.rangeStartMeters;
rangeEnd_meter = Params.vitalSignsParams.rangeEndMeters;

rangeBinStartIndex = floor(rangeStart_meter/(Params.dataPath.rangeBinSize_meter));   
rangeBinEndIndex   = floor(rangeEnd_meter/(Params.dataPath.rangeBinSize_meter));
numRangeBinProcessed = rangeBinEndIndex - rangeBinStartIndex + 1;

% Initialize the plots (outside the While Loop) for Real-time Display
hLineBreathing = plot(app.BreathingWfm,NaN,'color','b');
hLineHeartRate = plot(app.HeartRateWfm,NaN,'color','r');
hLinePhase = plot(app.PhaseUnwrapped,NaN,'color','k');
hLineRangeProfile = plot(app.RangeProfile,NaN);

rangeAxis = Params.dataPath.rangeIdxToMeters*[rangeBinStartIndex:rangeBinEndIndex];
xlabel(app.RangeProfile,'Range (meter)');
xlim(app.RangeProfile,[rangeStart_meter  rangeEnd_meter]);
xlim(app.BreathingWfm,[1 PLOT_DISPLAY_LENGTH])
xlim(app.HeartRateWfm,[1 PLOT_DISPLAY_LENGTH])  
xlim(app.PhaseUnwrapped,[1 PLOT_DISPLAY_LENGTH])

rangeBinValueUpdated = 0;
outHeartNew_CM = 0;
outBreathNew_CM = 0;
dataPlotPrev = 0;
dataPlotHeartPrev = 0;
dataPlotThresh = 50;

indices = uint16(zeros(3,3500));
indices1Temp = single(0);

HRVcount = 1;
frameCount = -2;
countdownClock = tic;
 
Timer = timer('ExecutionMode', 'fixedRate'); % Create the timer object
Timer.TimerFcn = @app.TimerRefresh; % Tell the timer to run TimerRefresh every second.
start(Timer);
while (~RESET_KEY_PRESSED && toc(countdownClock)< 300)
    if ~isempty(bytevec)
        startFramecou = framecou;
        
    if(app.EXIT_PRESSED)
        RESET_KEY_PRESSED = 1;
    end

    if(RESET_KEY_PRESSED)
       app.EXIT_PRESSED = 0;
       ss = sprintf('sensorStop \n');
       fprintf(spCliHandle, ss);
    end

     bytevec_cp(bytevec_cp_len+1:bytevec_cp_len+length(bytevec)) = bytevec;
     bytevec_cp_len = bytevec_cp_len + length(bytevec);
     bytevec=[];
     bytevecStr = char(bytevec_cp);
     magicOk = 0;
     startIdx = strfind(bytevecStr', char([2 1 4 3 6 5 8 7]));
       if ~isempty(startIdx)
            bytevec_cp(1: length(bytevec_cp)-(startIdx(1)-1)) = bytevec_cp(startIdx(1):end);
            bytevec_cp_len = bytevec_cp_len - (startIdx(1)-1);
            
            if bytevec_cp_len >= TOTAL_PAYLOAD_SIZE_BYTES
                magicOk = 1;
            else
                magicOk = 0;                
            end
        end
        

if(app.REFRESH_PRESSED)
stop(Timer);
framecountDummy = 0;
outBreathPlot = nan(1,PLOT_DISPLAY_LENGTH);outBreathPlot(1) = 0;
outHeartPlot  = nan(1,PLOT_DISPLAY_LENGTH);outHeartPlot(1) = 0;
outPhasePlot  = nan(1,PLOT_DISPLAY_LENGTH);outPhasePlot(1) = 0;
   ss = sprintf('guiMonitor %f %f %d %d \n',0,0,app.FFT_SPECTRAL_EST_ENABLE,1);
   fprintf(spCliHandle, ss);
   pause(.2);
outSumEnergyBreathWfm_thresh = app.ThresholdBreathing.Value;   % Threshold on the Breathing Waveform 
thresh_HeartCM = app.ThresholdHeart.Value;
app.REFRESH_PRESSED = 0;   
end

if (app.CLI_SEND_FLAG)        
        ss = sprintf('guiMonitor %f %f %d %d \n',0,0,app.FFT_SPECTRAL_EST_ENABLE,1);
        fprintf(spCliHandle, ss);
        app.CLI_SEND_FLAG = 0;   
        pause(.2);
end

    %keyboard
    if(magicOk == 1)
        if debug
           fprintf('Frame Interval = %.3f sec,  ', toc(tStart));
        end
   tStart = tic;
   
   % Remove the MagicWord
   bytevec_data = bytevec_cp;%(9:TOTAL_PAYLOAD_SIZE_BYTES); 
   globalCountTemp =  bytevec_data([INDEX_GLOBAL_COUNT: INDEX_GLOBAL_COUNT+3]);
   outGlobalCount = typecast(uint8([globalCountTemp]),'uint32');
   
   if (app.SaveData.Value)
    fwrite(fid_write_bin, bytevec_data,'uint8');      
   end
        
   framecountDummy = framecountDummy +1;
   cnt = mod(framecountDummy, PLOT_DISPLAY_LENGTH)+1;        
 
   rangeBinPhase = typecast(uint8([bytevec_data(INDEX_IN_RANGE_BIN_INDEX)]),'uint16');
   rangeProfileTemp     = bytevec_data(4*INDEX_IN_DATA_RANGE_PROFILE_START -3:length(bytevec_data));
    
   dataOut_AR14xx_float = typecast(uint8([bytevec_data]),'single');
   outPhase  = dataOut_AR14xx_float(INDEX_IN_DATA_PHASE);
   outBreath = dataOut_AR14xx_float(INDEX_IN_DATA_BREATHING_WAVEFORM);
   outHeart = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_WAVEFORM);
   breathRateEstPeak = dataOut_AR14xx_float(INDEX_IN_DATA_BREATHING_RATE_PEAK);
   heartRateEstPeak = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_RATE_EST_PEAK);
   breathRateEstFFT = dataOut_AR14xx_float(INDEX_IN_DATA_BREATHING_RATE_FFT);
   heartRateEstFFT = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_RATE_EST_FFT);
   outSumEnergyBreathWfm = dataOut_AR14xx_float(INDEX_IN_DATA_ENERGYWFM_BREATH);
   outSumEnergyHeartWfm = dataOut_AR14xx_float(INDEX_IN_DATA_ENERGYWFM_HEART);
   rangeBinValue = dataOut_AR14xx_float(INDEX_IN_DATA_RANGE_BIN_VALUE);
   motionFlag = dataOut_AR14xx_float(INDEX_IN_DATA_MOTION_DETECTION_FLAG);
   
   indices1 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_1);
   indices2 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_2);
   indices3 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_3);
   indices4 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_4);
   indices5 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_5);
   indices6 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_6);
   indices7 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_7);
   indices8 = dataOut_AR14xx_float(INDEX_IN_DATA_HEART_INDICES_8);
   if(~(indices1 == indices1Temp))
       if(frameCount < 0)
           frameCount = frameCount + 1;
           indices1Temp = indices1;
       else
           temp1 = typecast(indices1, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp1(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp1(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp1(i));
                   indices(3,HRVcount) = 1;
                   HRVcount = HRVcount + 1;
               end
           end
           indices1Temp = indices1;

           temp2 = typecast(indices2, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp2(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp2(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp2(i));
                   indices(3,HRVcount) = 2;
                   HRVcount = HRVcount + 1;
               end
           end

           temp3 = typecast(indices3, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp3(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp3(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp3(i));
                   indices(3,HRVcount) = 3;
                   HRVcount = HRVcount + 1;
               end
           end

           temp4 = typecast(indices4, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp4(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp4(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp4(i));
                   indices(3,HRVcount) = 4;
                   HRVcount = HRVcount + 1;
               end
           end

           temp5 = typecast(indices5, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp5(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp5(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp5(i));
                   indices(3,HRVcount) = 5;
                   HRVcount = HRVcount + 1;
               end
           end

           temp6 = typecast(indices6, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp6(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp6(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp6(i));
                   indices(3,HRVcount) = 6;
                   HRVcount = HRVcount + 1;
               end
           end

           temp7 = typecast(indices7, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp7(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp7(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp7(i));
                   indices(3,HRVcount) = 7;
                   HRVcount = HRVcount + 1;
               end
           end

           temp8 = typecast(indices8, 'uint8'); %[4,3,2,1]
           for i = 1:4
               if(temp8(i) ~= 0)
                   indices(1,HRVcount) = uint16(temp8(i)) + uint16(frameCount*256);
                   indices(2,HRVcount) = uint16(temp8(i));
                   indices(3,HRVcount) = 8;
                   HRVcount = HRVcount + 1;
               end
           end
           frameCount = frameCount + 1;
       end
   end

    %Plot range profile
    if (app.Plot_RangeProfile.Value)
     rangeProfile = single(typecast(uint8([rangeProfileTemp]),'uint16'));
     rangeProfile(rangeProfile>(2^15)) = rangeProfile(rangeProfile>(2^15))-2^16;
     rp_cplx = rangeProfile(1:2:end)+j*(rangeProfile(2:2:end));
     set(hLineRangeProfile,'XData',rangeAxis,'YData',abs(rp_cplx(1:numRangeBinProcessed)));
     [val ind]=max(abs(rp_cplx(1:numRangeBinProcessed)));
     rangeBinValue = val; 
    end

   %Check if plots are enabled on the GUI
   if(app.Plots_Enable.Value)
       
   if (cnt == PLOT_DISPLAY_LENGTH)
        outBreathPlot = nan(1,PLOT_DISPLAY_LENGTH);outBreathPlot(1) = 0;
        outHeartPlot  = nan(1,PLOT_DISPLAY_LENGTH);outHeartPlot(1) = 0;
        outPhasePlot  = nan(1,PLOT_DISPLAY_LENGTH);outPhasePlot(1) = outPhasePlot(PLOT_DISPLAY_LENGTH);
    end
       
    outBreathPlot(cnt) = outBreath;
    outHeartPlot(cnt)  = outHeart;
    outPhasePlot(cnt)  = outPhase;
  
    % Exponential Average
    rangeBinValuePrev = rangeBinValueUpdated;
    alpha = 0.1;
    rangeBinValueUpdated = alpha*(rangeBinValue) + (1-alpha)*rangeBinValuePrev;
    
     
    % Heart Rate Display Decision
    diffEst_heartRate = abs(heartRateEstFFT - heartRateEstPeak);    
    if (diffEst_heartRate < thresh_diffEst)
    heartRateEstDisplay = heartRateEstFFT; 
    else
    heartRateEstDisplay = heartRateEstPeak;
    end

heartRateEstDisplay_CircBuffer = circshift(heartRateEstDisplay_CircBuffer, [0 -1]);
heartRateEstDisplay_CircBuffer(LENGTH_BUFFER) = heartRateEstDisplay;
heartRateEstDisplayFinal = median(heartRateEstDisplay_CircBuffer);  
 
if (mod(cnt,valuesUpdateCount)==0)    % Displays after every valuesUpdateCount

    if(isnan(outSumEnergyBreathWfm)|| isinf(outSumEnergyBreathWfm))
        outSumEnergyBreathWfm = 99;
    end
    if(isnan(outSumEnergyHeartWfm) || isinf(outSumEnergyHeartWfm))
        outSumEnergyHeartWfm = 99;
    end
    
    app.Temp1Display.Value = double(breathRateEstPeak);%;
    app.Temp2Display.Value = double(heartRateEstPeak);
    app.Temp1Display_2.Value = double(breathRateEstFFT);
    app.Temp2Display_2.Value = double(heartRateEstFFT);
    app.MotionFlagEditField.Value = double(motionFlag);
    
    if (motionFlag ==1)
        app.MotionFlagEditField.BackgroundColor = 'red';
    else
        app.MotionFlagEditField.BackgroundColor = 'white';        
    end
        
     if ((rangeBinValueUpdated < rangeBinValueThresh) ||(outSumEnergyBreathWfm < outSumEnergyBreathWfm_thresh))
           app.BreathingRateNumberDisp.Value = 0; 
           app.BreathingRateNumberDisp.BackgroundColor = 'red'; 
     else
           app.BreathingRateNumberDisp.Value = double(breathRateEstFFT); 
           app.BreathingRateNumberDisp.BackgroundColor = 'white'; 
     end    
   
     if ((rangeBinValueUpdated < rangeBinValueThresh) || outSumEnergyHeartWfm < outSumEnergyHeartWfm_thresh )
           app.HeartRateNumberDisp.Value = 0; 
           app.HeartRateNumberDisp.BackgroundColor = 'red'; 
     else
           app.HeartRateNumberDisp.Value = double(heartRateEstDisplayFinal); 
           app.HeartRateNumberDisp.BackgroundColor = 'white'; 
     end
     
end

%% Update Waveform Plots   
   
   %Check if the corresponding plot is enabled on the GUI.
   if(app.Plot_Displacement.Value)
    set(hLinePhase,'YData',(outPhasePlot(1:cnt) - outPhasePlot(1)))
   end

   dataPlot = outBreathPlot(1:cnt) - outBreathPlot(1);
    
   if (abs(dataPlot) > dataPlotThresh)
    dataPlot = dataPlotPrev; 
   end
   dataPlotPrev = dataPlot;    
    
   set(hLineBreathing,'YData',dataPlot);
   tempData = dataPlot; tempData(isnan(dataPlot)) = [];
   dataLimMax = max(tempData);
   dataLimMin = min(tempData);
   if (dataLimMax < 1)
        dataLimMax = 1;
   end
   if (dataLimMin > -1)
        dataLimMin = -1;
   end
   ylim(app.BreathingWfm,[dataLimMin dataLimMax + 0.1])
       
   dataPlotHeart = outHeartPlot(1:cnt);
    
    if (abs(dataPlotHeart)> dataPlotThresh)
    dataPlotHeart = dataPlotHeartPrev; 
    end
    dataPlotHeartPrev = dataPlotHeart;
    
    set(hLineHeartRate,'YData', dataPlotHeart);
    drawnow; pause(0.05);   
    
   end          

   if debug
       fprintf('processing time %f secs \n',toc(tStart));
   end

   else
   if debug == 1
        fprintf('Serial port sync loss!  Frame number = %d \n',framecou);
    end
  end
        %Remove processed data
        while bytevec_cp_len > TOTAL_PAYLOAD_SIZE_BYTES
            bytevec_cp(1: length(bytevec_cp)-TOTAL_PAYLOAD_SIZE_BYTES) = bytevec_cp(TOTAL_PAYLOAD_SIZE_BYTES+1:end);
            bytevec_cp_len = bytevec_cp_len - TOTAL_PAYLOAD_SIZE_BYTES;
        end
        endFramecou = framecou; 
        tIdleStart = tic;
   end

  pause(0.001); 
 
  if(toc(tIdleStart) > 2*Params.frameCfg.framePeriodicity/1000)
        %fprintf('TIMEOUT %d\n', framecou);
        timeout_ctr=timeout_ctr+1;
        if debug == 1
            fprintf('Timeout counter = %d\n', timeout_ctr);
        end
        if Params.frameCfg.numFrames == 1
            %Single frame mode, start new frame        
            fprintf(spCliHandle, 'frameStart');
        end
        tIdleStart = tic;
   end
end

if(~RESET_KEY_PRESSED)
    % Calculate HRV data ------------
    indicesTemp = indices(1,:);
    indicesTemp = indicesTemp(1:find(indicesTemp,1,'last'));
    NN = zeros(1,length(indicesTemp)-1);
    for i = 1:length(NN)
        NN(i) = indicesTemp(i+1) - indicesTemp(i);
    end
    invalidEntries = NN < 11 | NN > 24;
    NN(invalidEntries) = [];
    NN = NN * 50; % convert number of samples to ms

    [SDNN, RMSSD, HTI] = HRV(NN);
    app.SDNN_val.Value = SDNN;
    app.RMSSD_val.Value = RMSSD;
    app.HTI_val.Value = HTI;

    % stop sensor
    ss = sprintf('sensorStop \n');
    fprintf(spCliHandle, ss);

    while(~EXIT_KEY_PRESSED)
        if(app.EXIT_PRESSED)
             EXIT_KEY_PRESSED = 1;
             app.EXIT_PRESSED = 0;
        end
    end
end
% -------------------------------

stop(Timer);
delete(Timer);

%close and delete handles before exiting
fclose(sphandle); %close com port
delete(sphandle);
fclose(spCliHandle);
delete(spCliHandle);   
  
fclose(fid_write_bin);
end

function [] = plotImage(obj, event)

global bytevec;
global framecou;
global TOTAL_PAYLOAD_SIZE_BYTES;

[bytevec, byteCount] = fread(obj, TOTAL_PAYLOAD_SIZE_BYTES, 'uint8');
framecou = framecou + 1;
%  fwrite(fid_write, bytevec,'uint8');
  %fprintf('\nREAD %d of %d Bytes, size of bytevec = %d\n\n', byteCount, TOTAL_PAYLOAD_SIZE_BYTES, length(bytevec));
end

function [] = readValue(obj, event)
global output_ppg_raw;
output_ppg_raw = fread(obj, 22, 'uint8');
end


function [] = dispError()
disp('error!');
end

function [sphandle] = configureSport(comportSnum)
    global TOTAL_PAYLOAD_SIZE_BYTES;

    if ~isempty(instrfind('Type','serial'))
        disp('Serial port(s) already open. Re-initializing...');
        delete(instrfind('Type','serial'));  % delete open serial ports.
    end
    comportnum_str=['COM' num2str(comportSnum)]
    sphandle = serial(comportnum_str,'BaudRate',921600);
    set(sphandle,'InputBufferSize', TOTAL_PAYLOAD_SIZE_BYTES);
    set(sphandle,'Timeout',10);
    set(sphandle,'ErrorFcn',@dispError);
    set(sphandle,'BytesAvailableFcnMode','byte');
    set(sphandle,'BytesAvailableFcnCount', TOTAL_PAYLOAD_SIZE_BYTES);
    set(sphandle,'BytesAvailableFcn',@plotImage);
    fopen(sphandle);
end

function [sphandle] = configureCliPort(comportPnum)
    %if ~isempty(instrfind('Type','serial'))
    %    disp('Serial port(s) already open. Re-initializing...');
    %    delete(instrfind('Type','serial'));  % delete open serial ports.
    %end
    comportnum_str=['COM' num2str(comportPnum)]
    sphandle = serial(comportnum_str,'BaudRate',115200);
    set(sphandle,'Parity','none')    
    set(sphandle,'Terminator','CR/LF')   
    fopen(sphandle);
end


function [sp_pulseOximeter] = configurePPG_Port(comportPnum)
    comportnum_str=['COM' num2str(comportPnum)]
    sp_pulseOximeter = serial(comportnum_str,'BaudRate',9600);  
    set(sp_pulseOximeter,'BytesAvailableFcnMode','byte');
    set(sp_pulseOximeter,'BytesAvailableFcnCount', 22);
    set(sp_pulseOximeter,'BytesAvailableFcn',@readValue);    
    
    fopen(sp_pulseOximeter);    
    fwrite(sp_pulseOximeter,strcat(hex2dec('01'),hex2dec('2A'),hex2dec('00'),hex2dec('00'),hex2dec('00'),hex2dec('01'),hex2dec('0D')));  % 1
    % Start Read command
    fwrite(sp_pulseOximeter,strcat(hex2dec('04'),hex2dec('0D')) ); 
end


function [clusterLabel ] = dbscan_1D(x, MinPts, Eps)

if(isrow(x))
    x = x';
end
DistanceMatrix = squareform(pdist(x, 'euclidean'));

numObs = length(x);             % Number of Observations
numCluster = 0;                 % Number of Clusters
numNeighbors = zeros(numObs,1); % num neighbours in range <= Eps
bool_isNoise = zeros(numObs,1); % class (noise = 1, otherwise 0);
clusterLabel = nan(numObs,1);   % Cluster label
ObsClassified = zeros(numObs,1);% Not classfied  = 0, Classified = 1

for indexObs =1:numObs

    if(~ObsClassified(indexObs))
        [temp, indexNeighbors]     = find(DistanceMatrix(indexObs, :) <= Eps);
        numNeighbors(indexObs)  = length(indexNeighbors);

        if(numNeighbors(indexObs) <= MinPts)              % Classified as Noise
            bool_isNoise(indexObs)  = 1;
            clusterLabel(indexObs)  = 0;
            ObsClassified(indexObs) = 1;
            continue;
        else                                              % Classfied as a cluster
            numCluster           = numCluster + 1;
            bool_isNoise(indexNeighbors)    = 1;
            clusterLabel(indexNeighbors) = numCluster;
            ObsClassified(indexNeighbors) = 1;
            % stack
            seedsIdx    = indexNeighbors(indexNeighbors ~= indexObs);
            while(~isempty(seedsIdx))
                currSeedStackIdx     = 1;
                currSeedIdx          = seedsIdx(currSeedStackIdx);
                [temp, indexNeighbors]  = find(DistanceMatrix(currSeedIdx, :) <= Eps);
                numNeighbors(currSeedIdx)    = length(indexNeighbors);

                if(numNeighbors(currSeedIdx)>=MinPts+1)
                    seedsIdx = [seedsIdx, indexNeighbors(ObsClassified(indexNeighbors) == 0)];
                    clusterLabel(indexNeighbors(ObsClassified(indexNeighbors) == 0 | bool_isNoise(indexNeighbors) == 1)) = numCluster;
                    bool_isNoise(indexNeighbors)  = 0;
                    ObsClassified(indexNeighbors) = 1;
                end
                seedsIdx(currSeedStackIdx) = [];
            end
        end
    end
end


end


%Read relevant CLI parameters and store into P structure
function [P] = parseCfg(cliCfg)
global TOTAL_PAYLOAD_SIZE_BYTES
global LENGTH_HEADER_BYTES;
global MMWDEMO_OUTPUT_MSG_SEGMENT_LEN;
global LENGTH_TLV_MESSAGE_HEADER_BYTES;
global LENGTH_DEBUG_DATA_OUT_BYTES;


    P=[];
    for k=1:length(cliCfg)
        C = strsplit(cliCfg{k});
        if strcmp(C{1},'channelCfg')
            P.channelCfg.txChannelEn = str2num(C{3});
            P.dataPath.numTxAzimAnt = bitand(bitshift(P.channelCfg.txChannelEn,0),1) +...
                                      bitand(bitshift(P.channelCfg.txChannelEn,-2),1);
            P.dataPath.numTxElevAnt = bitand(bitshift(P.channelCfg.txChannelEn,-1),1);
            P.channelCfg.rxChannelEn = str2num(C{2});
            P.dataPath.numRxAnt = bitand(bitshift(P.channelCfg.rxChannelEn,0),1) +...
                                  bitand(bitshift(P.channelCfg.rxChannelEn,-1),1) +...
                                  bitand(bitshift(P.channelCfg.rxChannelEn,-2),1) +...
                                  bitand(bitshift(P.channelCfg.rxChannelEn,-3),1);
            P.dataPath.numTxAnt = P.dataPath.numTxElevAnt + P.dataPath.numTxAzimAnt;
                                
        elseif strcmp(C{1},'dataFmt')
        elseif strcmp(C{1},'profileCfg')
            P.profileCfg.startFreq = str2num(C{3});
            P.profileCfg.idleTime =  str2num(C{4});
            P.profileCfg.rampEndTime = str2num(C{6});
            P.profileCfg.freqSlopeConst = str2num(C{9});
            P.profileCfg.numAdcSamples = str2num(C{11});
            P.profileCfg.digOutSampleRate = str2num(C{12}); %uints: ksps
        elseif strcmp(C{1},'chirpCfg')
        elseif strcmp(C{1},'frameCfg')
            P.frameCfg.chirpStartIdx = str2num(C{2});
            P.frameCfg.chirpEndIdx = str2num(C{3});
            P.frameCfg.numLoops = str2num(C{4});
            P.frameCfg.numFrames = str2num(C{5});
            P.frameCfg.framePeriodicity = str2num(C{7});
        elseif strcmp(C{1},'guiMonitor')
          P.guiMonitor.flag1 =  str2num(C{2});
          P.guiMonitor.flag2 = str2num(C{3});    
          P.guiMonitor.rangeAzimuthHeatMap = str2num(C{4});
          P.guiMonitor.rangeDopplerHeatMap = str2num(C{5});
        
        elseif strcmp(C{1},'vitalSignsCfg')
          P.vitalSignsParams.rangeStartMeters =  str2num(C{2});
          P.vitalSignsParams.rangeEndMeters = str2num(C{3});    
          P.vitalSignsParams.winLenBreath = str2num(C{4});
          P.vitalSignsParams.winLenHeart  = str2num(C{5});
        end
        
    end
    P.dataPath.numChirpsPerFrame = (P.frameCfg.chirpEndIdx -...
                                            P.frameCfg.chirpStartIdx + 1) *...
                                            P.frameCfg.numLoops;
    P.dataPath.numDopplerBins = P.dataPath.numChirpsPerFrame / P.dataPath.numTxAnt;
    P.dataPath.numRangeBins = pow2roundup(P.profileCfg.numAdcSamples);
    P.dataPath.rangeResolutionMeters = 3e8 * P.profileCfg.digOutSampleRate * 1e3 /...
                     (2 * P.profileCfg.freqSlopeConst * 1e12 * P.profileCfg.numAdcSamples);
    P.dataPath.rangeIdxToMeters = 3e8 * P.profileCfg.digOutSampleRate * 1e3 /...
                     (2 * P.profileCfg.freqSlopeConst * 1e12 * P.dataPath.numRangeBins);
    P.dataPath.dopplerResolutionMps = 3e8 / (2*P.profileCfg.startFreq*1e9 *...
                                        (P.profileCfg.idleTime + P.profileCfg.rampEndTime) *...
                                        1e-6 * P.dataPath.numDopplerBins * P.dataPath.numTxAnt);
   P.dataPath.chirpDuration_us = (1e3*P.profileCfg.numAdcSamples)/(P.profileCfg.digOutSampleRate);
   freqSlopeConst_temp = 48*P.profileCfg.freqSlopeConst* 2^26 * 1e3/((3.6*1e9)*900);  % To match the C-code 
   
   P.dataPath.chirpBandwidth_kHz = (freqSlopeConst_temp)*(P.dataPath.chirpDuration_us );
   
   numTemp = (P.dataPath.chirpDuration_us)*(P.profileCfg.digOutSampleRate)*(3e8) ;
   denTemp = 2*(P.dataPath.chirpBandwidth_kHz);
   P.dataPath.rangeMaximum =  numTemp/(denTemp*1e9);
   P.dataPath.rangeBinSize_meter = P.dataPath.rangeMaximum/(P.dataPath.numRangeBins);
   rangeStart_Index = floor(P.vitalSignsParams.rangeStartMeters/P.dataPath.rangeBinSize_meter);
   rangeEnd_Index   = floor(P.vitalSignsParams.rangeEndMeters/P.dataPath.rangeBinSize_meter);
   P.dataPath.lambdaCenter_mm = (3e8/(P.profileCfg.startFreq))/1e6;
   %Calculate monitoring packet size
   numRangeBinProcessed = rangeEnd_Index - rangeStart_Index + 1;
   TOTAL_PAYLOAD_SIZE_BYTES = LENGTH_HEADER_BYTES;
   TOTAL_PAYLOAD_SIZE_BYTES = TOTAL_PAYLOAD_SIZE_BYTES+...
              LENGTH_TLV_MESSAGE_HEADER_BYTES + (4*numRangeBinProcessed);
   TOTAL_PAYLOAD_SIZE_BYTES = TOTAL_PAYLOAD_SIZE_BYTES+...
              LENGTH_TLV_MESSAGE_HEADER_BYTES+LENGTH_DEBUG_DATA_OUT_BYTES;
    
    % Padding 
    if (mod(TOTAL_PAYLOAD_SIZE_BYTES,MMWDEMO_OUTPUT_MSG_SEGMENT_LEN)~=0)
        paddingFactor = ceil(TOTAL_PAYLOAD_SIZE_BYTES/ MMWDEMO_OUTPUT_MSG_SEGMENT_LEN);
        TOTAL_PAYLOAD_SIZE_BYTES = MMWDEMO_OUTPUT_MSG_SEGMENT_LEN*paddingFactor;
    end
    
end

function [INDEX_OUT] = TRANSLATE_INDEX(OFFSET, INDEX_IN)
% global LENGTH_OFFSET_BYTES;
% global LENGTH_TLV_MESSAGE_HEADER_BYTES;
INDEX_OUT  = (OFFSET + INDEX_IN*4)/4;
end

function [y] = pow2roundup (x)
    y = 1;
    while x > y
        y = y * 2;
    end
end

function [SDNN, RMSSD, HTI] = HRV(NN)
    SDNN = std(NN);
    
    RMSSD = 0;
    for i = 1:length(NN)-1
        RMSSD = RMSSD + (NN(i) - NN(i+1))^2;
    end
    RMSSD = RMSSD/length(NN);
    RMSSD = sqrt(RMSSD);
    
    [H, ~] = histcounts(NN, linspace(500,1250,96));
    HTI = length(NN)/max(H);
end









