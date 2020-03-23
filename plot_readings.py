import matplotlib
import matplotlib.pyplot as plt
import sys
import random
import math


if len(sys.argv) != 2:
    print(sys.argv[0], "<file name>")
    exit(2)


with open(sys.argv[1], 'r') as f:

    # read preamble
    # assumes a colon separate the name and the value
    channel_description = f.readline().strip()
    start_time = float(f.readline().split(':')[1].strip())
    channels = f.readline().split(':')[1].strip()
    sample_rate = int(f.readline().split(':')[1].strip())
    samp_per_second = int(f.readline().split(':')[1].strip())
    num_timestamps = int(f.readline().split(':')[1].strip())
    num_samples = int(f.readline().split(':')[1].strip())


    f.readline() # section break empty section

    timestamps = []
    raw_data = []
    readings = []

    # process timestamps
    for i in range(num_timestamps):
        line = f.readline()
        time, message = line.split('|')
        time = (float(time) - start_time) / 1000000000
        timestamps.append({'time': time, 'message': message})

    f.readline() # section break empty section

    # process power readings
    for i in range(math.ceil(num_samples/sample_rate)):
        line = f.readline()
        data = [float(measurement.strip()) for measurement in line.split(',')]
        raw_data.append(data)

        # - time is calculated based on readings index relative to the
        #   start of the session
        # - TODO: TAKE OUT MAGIC NUMBER FOR DATA INDEX RANGE
        readings.append(
            {'time': sample_rate/samp_per_second * (i+1),
            'power': sum(data[-6::])})

x_max = max(timestamps, key=lambda x: x['time'])['time'] 
y_max = max(readings, key=lambda x: x['power'])['power'] 

# Create main canvas and add subplots
fig = plt.figure()
ax = fig.add_subplot(111)

# add vertical lines for timestamps
for timestamp in timestamps:
    ax.axvline(x=timestamp['time'], linestyle='--')
    ax.text(timestamp['time'], random.randint(0,int(y_max)), timestamp['message'], fontsize=6)

# plot values
ax.plot([i['time'] for i in readings], [i['power'] for i in readings], 'b', linestyle='-')

# set labels and title
ax.set_title(sys.argv[1])
ax.set_xlabel('Time (seconds)')
ax.set_ylabel('Power (joules)')

# set axis scale
ax.axis([ 0, x_max + 1, 0, y_max + 10])


# print(readings)
# print(timestamps)

# show graphs
plt.show()

