#include "nidaqmxeventhandler.h"
#include <iostream>

NIDAQmxEventHandler::NIDAQmxEventHandler(void){};
NIDAQmxEventHandler::~NIDAQmxEventHandler(void){};

NIDAQmxEventHandler::NIDAQmxEventHandler(std::string logFilePath) {
  logFile = logFilePath;
  writer = fstream(logFile);
}

// TODO: DETERMINE ACCURACY OF THESE CONSTANTS
// coresponds to ATX 24-pin (maybe..?)
float64 nidaq_chan_volts[] = {
    3.3,    3.3,   5.00, 5.00,
    12.00,  12.00, 3.3,  3.3, /* NIDAQ1 MOD1 channel 0-7 */
    -12.00, 5.00,  5.00, 5.00, /* NIDAQ1 MOD1 channel 16-19 */
    12.0,1.0,12.0,12.0,12.0,12.0
};

void NIDAQmxEventHandler::startHandler() {
  std::cout << "Event Handler: Starting...\n";
  timestamps.emplace_back("Starting Session...", timestamp);

  writer << CHANNEL_DESCRIPTION << std::endl;
  writer << "START TIME: " << timestamps.front().second() << std::endl;
  writer << "NUMBER OF CHANNELS: " << NUM_CHANNELS << std::endl;
  writer << "SAMPLES TAKEN: " << totalSamplesRead << std::endl;
  writer << "SAMPLE RATE: " << SAMPLE_RATE << std::endl;
  writer << "NUMBER OF TIMESTAMPS: " << timestamps.size() << std::endl;

  int32 error = 0;
  taskHandle = 0;
  char errBuff[ERRBUFF] = {'\0'};

  /*********************************************/
  // DAQmx Configure Code
  /*********************************************/
  DAQmxErrChk(DAQmxCreateTask("", &taskHandle));

  DAQmxErrChk(DAQmxCreateAIVoltageChan(
      taskHandle, CHANNEL_DESCRIPTION, "",
      DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL));

  DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle, NULL, 1000.0, DAQmx_Val_Rising,
                                    DAQmx_Val_ContSamps, 16000));

  DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(
      taskHandle, DAQmx_Val_Acquired_Into_Buffer, SAMPLE_RATE, 0,
      EveryNCallback, NULL));

  DAQmxErrChk(DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));

  /*********************************************/
  // DAQmx Start Code
  /*********************************************/
  DAQmxErrChk(DAQmxStartTask(taskHandle));

Error:
  if (DAQmxFailed(error)) {
    DAQmxGetExtendedErrorInfo(errBuff, ERRBUFF);
    printf("DAQmx Error: %s\n", errBuff);
  }
}

void NIDAQmxEventHandler::tagHandler(uint64_t timestamp, std::string tag) {
  std::cout << "Event Handler: Tag" << std::endl;
  timestamps.emplace_back(tag, timestamp);
}

void NIDAQmxEventHandler::endHandler(uint64_t timestamp) {
  timestamps.emplace_back("Ending Session...", timestamp);
  std::cout << "Event Handler: Ending Session..." << std::endl;
  DAQmxStopTask(taskHandle);
  DAQmxClearTask(taskHandle);

  for(auto& entry : timestamps){
      writer << entry.second() << "\t" << entry.first() << std::endl;
  }

}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle,
                                 int32 everyNsamplesEventType, uInt32 nSamples,
                                 void *callbackData) {
  int32 error = 0;
  char errBuff[2048] = {'\0'};
  int32 samplesRead = 0;
  float64 data[BUFFER_SIZE];
  std::string dataString;
  float64 channels[NUM_CHANNELS];
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
           (int)(totalSamplesRead += samplesRead));
    fflush(stdout);
  }

  for (size_t index = 0; index < BUFFER_SIZE; index++) {
    dataString += std::to_string(data[index]) + " ";
  }

  writer << dataString << std::endl;
  nidaq_diff_volt_to_power(channels, NUM_CHANNELS);

Error:
  if (DAQmxFailed(error)) {
    // Get and print error information
    DAQmxGetExtendedErrorInfo(errBuff, ERRBUFF);

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

void nidaqDiffVoltToPower(float64 *ChanReading, int num_chan) {
  int i;
  float64 milli_powers[num_chan];

  std::cout << "ChanReading" << std::endl;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    std::cout << i << ": " << ChanReading[i] << std::endl;
  }

  std::cout << "Stupid Nums" << std::endl;
  for (i = 0; i < num_chan; i++) {
    milli_powers[i] = (ChanReading[i] / NIDAQ_CHAN_RESISTOR) *
                      (nidaq_chan_volts[i] - ChanReading[i]);
    std::cout << i << ": " << milli_powers[i] << std::endl;
  }
  std::cout << std::endl;
}