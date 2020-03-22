#include "nidaqmxeventhandler.h"
#include <iostream>

NIDAQmxEventHandler::NIDAQmxEventHandler(void) {}

NIDAQmxEventHandler::~NIDAQmxEventHandler(void) {
  // free the list of voltages if present
  delete config.channelVoltages;
  // close file stream
  writer.close();
}

/**
 * Constructor with provided logfile
 *
 * @param logFilePath the file path to the logfile where power readings and time
 * stamps will be written
 */
NIDAQmxEventHandler::NIDAQmxEventHandler(std::string logFilePath) {
  logFile = logFilePath;
  writer.open(logFile, std::fstream::out);
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

  writer << "CHANNEL DESCRIPTION: " << config.channelDescription << std::endl;
  writer << "START TIME: " << timestamps[0].second << std::endl;
  writer << "NUMBER OF CHANNELS: " << config.numChannels << std::endl;
  writer << "SAMPLE RATE: " << config.sampleRate << std::endl;
  writer << std::endl;

  int32 error = 0;
  taskHandle = 0;
  char errBuff[2048] = {'\0'};

  /*********************************************/
  // DAQmx Configure Code
  /*********************************************/
  DAQmxErrChk(DAQmxCreateTask("", &taskHandle));

  DAQmxErrChk(DAQmxCreateAIVoltageChan(
      taskHandle, config.channelDescription.c_str(), "", DAQmx_Val_Cfg_Default,
      -10.0, 10.0, DAQmx_Val_Volts, NULL));

  DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle, NULL, 1000.0, DAQmx_Val_Rising,
                                    DAQmx_Val_ContSamps, 16000));

  DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(
      taskHandle, DAQmx_Val_Acquired_Into_Buffer, config.sampleRate, 0,
      EveryNCallback, (void *)this));

  DAQmxErrChk(DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));

  /*********************************************/
  // DAQmx Start Code
  /*********************************************/
  DAQmxErrChk(DAQmxStartTask(taskHandle));

Error:
  if (DAQmxFailed(error)) {
    DAQmxGetExtendedErrorInfo(errBuff, 2048);
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
  for (auto &entry : timestamps) {
    writer << entry.second << "\t" << entry.first << std::endl;
  }

  writer << std::endl;
  writer << "NUMBER OF TIMESTAMPS: " << timestamps.size() << std::endl;
  writer << "TOTAL SAMPLES TAKEN: " << totalSamplesRead << std::endl;
}

void NIDAQmxEventHandler::configure(Configuration configuration) {
  config.numChannels =
      stoi(configuration.get("NIDAQmxNumChannels"), nullptr, 10);
  config.sampleRate = stoi(configuration.get("NIDAQmxSampleRate"), nullptr, 10);
  config.bufferSize = config.numChannels * config.sampleRate;
  config.channelDescription = configuration.get("NIDAQmxChannelDescription");
  config.channelVoltages =
      stringToDoubleArray(configuration.get("NIDAQmxChannelVoltages"));
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
                                 void *callbackData) {
  NIDAQmxEventHandler *handler = (NIDAQmxEventHandler *)callbackData;
  int numChannels = handler->config.numChannels;
  uInt32 bufferSize = handler->config.bufferSize;

  int32 error = 0;
  char errBuff[2048] = {'\0'};
  int32 samplesRead = 0;
  float64 data[bufferSize];
  std::string dataString;
  float64 channels[numChannels];
  float64 powerReadings[numChannels];

  /*********************************************/
  // DAQmx Read Code
  /*********************************************/
  DAQmxErrChk(DAQmxReadAnalogF64(taskHandle, -1, 0, DAQmx_Val_GroupByChannel,
                                 data, bufferSize, &samplesRead, NULL));
  if (samplesRead > 0) {
    // Get the average reading for each channel
    for (int i = 0; i < numChannels; i++) {
      channels[i] = 0.0;
      for (int j = 0; j < samplesRead; j++) {
        channels[i] += data[j + i * samplesRead];
      }
      channels[i] /= samplesRead;
    }
  }

  if (samplesRead > 0) {
    printf("Acquired %d samples. Total %d\r", samplesRead,
           handler->totalSamplesRead += samplesRead);
    fflush(stdout);
  }

  nidaqDiffVoltToPower(powerReadings, channels, handler->config.channelVoltages,
                       numChannels);

  for (int index = 0; index < numChannels; index++) {
    dataString += std::to_string(powerReadings[index]) + " ";
  }

  handler->writer << dataString << std::endl;

Error:
  if (DAQmxFailed(error)) {
    // Get and print error information
    DAQmxGetExtendedErrorInfo(errBuff, 2048);

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
                               void *callbackData) {
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
 * @param result location for storage of results
 * @param readings voltage differential measurements from the ni meter
 * @param voltages the voltage for each cable that the ni meter is reading
 * @param numChannels the number of channels the ni meter is reading from
 */
void nidaqDiffVoltToPower(float64 *result, float64 *readings, float64 *voltages,
                          size_t numChannels) {
  for (size_t i = 0; i < numChannels; i++) {
    result[i] =
        (readings[i] / NIDAQ_CHAN_RESISTOR) * (voltages[i] - readings[i]);
  }
}