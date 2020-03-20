#include "nidaqmxeventhandler.h"
#include <iostream>

NIDAQmxEventHandler::NIDAQmxEventHandler(void){}
NIDAQmxEventHandler::~NIDAQmxEventHandler(void) {
  // free the list of voltages if present
  delete config.channelVoltages;
  // close file stream
  writer.close();
}

NIDAQmxEventHandler::NIDAQmxEventHandler(std::string logFilePath) {
  logFile = logFilePath;
  writer.open(logFile, std::fstream::out);
}

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

void NIDAQmxEventHandler::tagHandler(uint64_t timestamp, std::string tag) {
  timestamps.emplace_back(tag, timestamp);
}

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

void nidaqDiffVoltToPower(float64 *result, float64 *readings, float64 *voltages,
                          size_t numChannels) {
  for (size_t i = 0; i < numChannels; i++) {
    result[i] =
        (readings[i] / NIDAQ_CHAN_RESISTOR) * (voltages[i] - readings[i]);
  }
}