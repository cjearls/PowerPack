#ifndef NI_DAQ_MX_EVENT_HANDLER_H
#define NI_DAQ_MX_EVENT_HANDLER_H

#include <NIDAQmx.h>
#include "eventhandler.h"
#include "functionapi.h"

/*********************************************************************
 *
 * ANSI C Example program:
 *    ContAcq-IntClk.c
 *
 * Example Category:
 *    AI
 *
 * Description:
 *    This example demonstrates how to acquire a continuous amount of
 *    data using the DAQ device's internal clock.
 *
 * Instructions for Running:
 *    1. Select the physical channel to correspond to where your
 *       signal is input on the DAQ device.
 *    2. Enter the minimum and maximum voltage range.
 *    Note: For better accuracy try to match the input range to the
 *          expected voltage level of the measured signal.
 *    3. Set the rate of the acquisition. Also set the Samples per
 *       Channel control. This will determine how many samples are
 *       read at a time. This also determines how many points are
 *       plotted on the graph each time.
 *    Note: The rate should be at least twice as fast as the maximum
 *          frequency component of the signal being acquired.
 *
 * Steps:
 *    1. Create a task.
 *    2. Create an analog input voltage channel.
 *    3. Set the rate for the sample clock. Additionally, define the
 *       sample mode to be continuous.
 *    4. Call the Start function to start the acquistion.
 *    5. Read the data in the EveryNCallback function until the stop
 *       button is pressed or an error occurs.
 *    6. Call the Clear Task function to clear the task.
 *    7. Display an error if any.
 *
 * I/O Connections Overview:
 *    Make sure your signal input terminal matches the Physical
 *    Channel I/O control. For further connection information, refer
 *    to your hardware reference manual.
 *
 *********************************************************************/

// TODO: DETERMINE ACCURACY OF THIS CONSTANTS
#define NIDAQ_CHAN_RESISTOR 0.003  // currently don't know what this is for..

// Allows DAQmx functions to respond to errors with their own error block
#define DAQmxErrChk(functionCall)          \
  if (DAQmxFailed(error = (functionCall))) \
  { goto Error; }                            

// int NIMeasure(void); I don't think this is used

// called after measurements have concluded
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status,
                               void *callbackData);

// called after every n samples are measured by the ni meter
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle,
                                 int32 everyNsamplesEventType, uInt32 nSamples,
                                 void *callbackData);
void nidaqDiffVoltToPower(float64 *result, float64 *readings, float64 *voltages,
                          size_t numChannels);

// Wrapper struct to bundle NIDAQmx related configuration options
struct NIDAQmxConfig {
  // Number of channels being used
  int numChannels;
  // Number of samples taken before a callback is triggered
  int32 sampleRate;
  // Number of samples taken per second
  float64 samplesPerSecond;
  // Minimum buffer size needed to hold channel data in a callback
  uInt32 bufferSize;
  // Description of channels being used
  std::string channelDescription;
  // Voltages for each channel, used when converting readings from voltage to
  // power
  double *channelVoltages;
};

class NIDAQmxEventHandler : public eventHandler {
 public:
  int32 totalSamplesRead = 0;
  // configuration options
  NIDAQmxConfig config;
  NIDAQmxEventHandler(void);
  // constructor with given logfile
  NIDAQmxEventHandler(std::string logFilePath);
  // default destructor
  virtual ~NIDAQmxEventHandler();
  void configure(Configuration configuration);
  void storeReadings(float64* readings, size_t numChannels);

  // handles start event
  void startHandler(uint64_t timestamp);
  // handles tag event
  void tagHandler(uint64_t timestamp, std::string tag);
  // handles end event
  void endHandler(uint64_t timestamp);

 private:
  // internal handle for nidaq measurement task
  TaskHandle taskHandle;
  // vector holding vectors of measurements
  std::vector<std::vector<float64>> measurements;
  // print results to file
  void printResults();
};

#endif