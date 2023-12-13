import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
from datetime import datetime

fig = plt.figure()
ax1 = fig.add_subplot(2,1,1)
ax2 = fig.add_subplot(2,1,2)

def animate(i):
    pullData = open("sampleText2.txt","r").read()
    dataArray = pullData.split('\n')
    temp_ar = []
    humidity_ar = []
    timeinfo_ar = []
    for eachLine in dataArray[len(dataArray)-11:]:
        if len(eachLine)>1:
            _, humidity, _, temp, _, timeinfo= eachLine.split(' ')
            temp_ar.append(float(temp))
            humidity_ar.append(float(humidity))
            timeinfo_ar.append(datetime.fromisoformat(timeinfo.rstrip('Z')))
    ax1.clear()
    ax1.plot(timeinfo_ar, temp_ar)
    ax1.set_ylabel("Temperature Celsius")
    ax2.clear()
    ax2.set_ylabel("Relative Humidity")
    ax2.plot(timeinfo_ar, humidity_ar)
ani = animation.FuncAnimation(fig, animate, interval=1000)
fig.suptitle('IOT Device 2', fontsize=12)
plt.show()