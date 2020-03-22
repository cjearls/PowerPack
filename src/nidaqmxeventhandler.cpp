#include "nidaqmxeventhandler.h"
#include <iostream>

/**
 * Default Constructor
 */
NIDAQmxEventHandler::NIDAQmxEventHandler(void){};

/**
 * Default Destructor
 */
NIDAQmxEventHandler::~NIDAQmxEventHandler(void) { writer.close(); };

/**
 * Constructor with provided logfile
 *
 * @param logFilePath the file path to the logfile where power readings and time
 * stamps will be written
 */
NIDAQmxEventHandler::NIDAQmxEventHandler(std::string logFilePath) {
  logFile = logFilePath;
  std::cout << logFile << std::endl;
  writer = std::fstream(logFile, std::fstream::out);
}

// TODO: DETERMINE ACCURACY OF THESE CONSTANTS
// coresponds to ATX 24-pin (maybe..?)
float64 nidaq_chan_volts[] = {
    3.3,    3.3,   5.00, 5.00,            /* NIDAQ1 MOD1 channel 0-3 */
    12.00,  12.00, 3.3,  3.3,             /* NIDAQ1 MOD1 channel 4-7 */
    -12.00, 5.00,  5.00, 5.00,            /* NIDAQ1 MOD1 channel 16-19 */
    12.0,   1.0,   12.0, 12.0, 12.0, 12.0 /* NIDAQ1 MOD3 channel 0-5 */
};

/**
 * Performs meter setup. Called after recieving the "start session"
 * communication
 *
 * @param timestamp epoch time at which the session is started
 */
void NIDAQmxEventHandler::startHandler(uint64_t timestamp) {
  timestamps.emplace_back("Starting Session...", timestamp);

  writer << "CHANNEL DESCRIPTION: " << CHANNEL_DESCRIPTION << std::endl;
  writer << "START TIME: " << timestamps[0].second << std::endl;
  writer << "NUMBER OF CHANNELS: " << NUM_CHANNELS << std::endl;
  writer << "SAMPLE RATE: " << SAMPLE_RATE << std::endl;
  writer << "NUMBER OF TIMESTAMPS: " << timestamps.size() << std::endl;
  writer << std::endl;

  int32 error = 0;
  taskHandle = 0;
  char errBuff[ERRBUFF_SIZE] = {'\0'};

  /*********************************************/
  // DAQmx Configure Code
  /*********************************************/
  DAQmxErrChk(DAQmxCreateTask("", &taskHandle));

  DAQmxErrChk(DAQmxCreateAIVoltageChan(taskHandle, CHANNEL_DESCRIPTION, "",
                                       DAQmx_Val_Cfg_Default, -10.0, 10.0,
                                       DAQmx_Val_Volts, NULL));

  DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle, NULL, 1000.0, DAQmx_Val_Rising,
                                    DAQmx_Val_ContSamps, 16000));

  DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(
      taskHandle, DAQmx_Val_Acquired_Into_Buffer, SAMPLE_RATE, 0,
      EveryNCallback, (void*)this));

  DAQmxErrChk(DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));

  /*********************************************/
  // DAQmx Start Code
  /*********************************************/
  DAQmxErrChk(DAQmxStartTask(taskHandle));

Error:
  if (DAQmxFailed(error)) {
    DAQmxGetExtendedErrorInfo(errBuff, ERRBUFF_SIZE);
    printf("DAQmx Error: %s\n", errBuff);
  }
}

/**
 * Adds the given timestamp, tag pair to the collection of timestamps.  Called
 * when an "tag" communication is recieved
 *
 * @param timestamp epoch time of the event occuring
 * @param tag a string describing the event that was timestamped
 */
void NIDAQmxEventHandler::tagHandler(uint64_t timestamp, std::string tag) {
  timestamps.emplace_back(tag, timestamp);
}

/**
 * Perfroms meter shutdown, cleanup, and final io.  Called when an "end session"
 * communication is recieved
 *
 * @param timestamp epoch time of the end of the session
 */
void NIDAQmxEventHandler::endHandler(uint64_t timestamp) {
  timestamps.emplace_back("Ending Session...", timestamp);
  DAQmxStopTask(taskHandle);
  DAQmxClearTask(taskHandle);

  // print timestamps
  writer << std::endl;
  for (auto& entry : timestamps) {
    writer << entry.second << "\t" << entry.first << std::endl;
  }
}

/**
 * Averages n samples, converts them to power, and stores them in an
 * intermediate buffer.  Called after n samples are read by the meter
 *
 * @param taskHandle the task handle of the current measuring task
 * @param everyNsamplesEventType code indicating the type of functionality this
 * function contains.
 * @param nSamples the number of samples read each time before this function is
 * called
 * @param callbackData a way to access the event handler that triggered this
 * callback function
 */
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle,
                                 int32 everyNsamplesEventType, uInt32 nSamples,
                                 void* callbackData) {
  int32 error = 0;
  char errBuff[2048] = {'\0'};
  int32 samplesRead = 0;
  float64 data[BUFFER_SIZE];
  std::string dataString;
  float64 channels[NUM_CHANNELS];
  NIDAQmxEventHandler* handler = (NIDAQmxEventHandler*)callbackData;
  /*********************************************/
  // DAQmx Read Code
  /*********************************************/
  DAQmxErrChk(DAQmxReadAnalogF64(taskHandle, -1, 0, DAQmx_Val_GroupByChannel,
                                 data, BUFFER_SIZE, &samplesRead, NULL));
  if (samplesRead > 0) {
    // Get the average reading for each channel
    for (size_t i = 0; i < NUM_CHANNELS; i++) {
      channels[i] = 0.0;
      for (size_t j = 0; j < samplesRead; j++) {
        channels[i] += data[j + i * samplesRead];
      }
      channels[i] /= samplesRead;
    }
  }

  if (samplesRead > 0) {
    printf("Acquired %d samples. Total %d\r", (int)samplesRead,
           (int)(handler->totalSamplesRead += samplesRead));
    fflush(stdout);
  }

  for (size_t index = 0; index < BUFFER_SIZE; index++) {
    dataString += std::to_string(data[index]) + " ";
  }

  handler->writer << dataString << std::endl;
  voltageDifferentialToPower(channels, NUM_CHANNELS, nidaq_chan_volts);

Error:
  if (DAQmxFailed(error)) {
    // Get and print error information
    DAQmxGetExtendedErrorInfo(errBuff, ERRBUFF_SIZE);

    /*********************************************/
    // DAQmx Stop Code
    /*********************************************/
    DAQmxStopTask(taskHandle);
    DAQmxClearTask(taskHandle);
    printf("DAQmx Error: %s\n", errBuff);
  }
  return 0;
}

/**
 * Checks the err status.  Performed after completion of measuring task
 *
 * @param taskHandle the task handle of the current measuring task
 * @param status status code
 * @param callbackData a way to access the event handler that triggered this
 * callback function
 */
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status,
                               void* callbackData) {
  int32 error = 0;
  char errBuff[2048] = {'\0'};

  // Check to see if an error stopped the task.
  DAQmxErrChk(status);

Error:
  if (DAQmxFailed(error)) {
    DAQmxGetExtendedErrorInfo(errBuff, 2048);
    DAQmxClearTask(taskHandle);
    printf("DAQmx Error: %s\n", errBuff);
  }
  return 0;
}

/**
 * Converts voltage differential to power
 *
 * @param chanReading voltage differential measurements from the ni meter
 * @param numChannels the number of channels the ni meter is reading from
 * @param cableVoltages the voltage for each cable that the ni meter is reading
 * from
 */
void voltageDifferentialToPower(float64* ChanReading, int numChannels,
                                float64* cableVoltages) {
  float64 milli_powers[numChannels];
  for (size_t i = 0; i < numChannels; i++) {
    milli_powers[i] = (ChanReading[i] / NIDAQ_CHAN_RESISTOR) *
                      (cableVoltages[i] - ChanReading[i]);
  }
}