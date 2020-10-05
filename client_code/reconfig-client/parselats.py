#!/usr/bin/python3

import sys
import os
import numpy as np
from scipy import stats
import pdb

class Lat(object):
    def __init__(self, fileName):
        f = open(fileName, 'rb')
        a = np.fromfile(f, dtype=np.uint64)
        self.reqTimes = a.reshape((a.shape[0], 1))
        f.close()

    def parseQueueTimes(self):
        return self.reqTimes[:, 0]

    def parseSvcTimes(self):
        return self.reqTimes[:, 1]

    def parseSojournTimes(self):
        return self.reqTimes[:, 0]


if __name__ == '__main__':
    def getLatPct(typeOfLats, latsFile):
        assert os.path.exists(latsFile)

        latsObj = Lat(latsFile)
        print("======= {} ======".format(latsFile))
        if typeOfLats == '--latency':
            # qTimes = [l/1e6 for l in latsObj.parseQueueTimes()]
            # svcTimes = [l/1e6 for l in latsObj.parseSvcTimes()]
            sjrnTimes = [l/1e3 for l in latsObj.parseSojournTimes()]

            print('Num of Requests: {}'.format(len(sjrnTimes)))
            mean = np.mean(sjrnTimes)
            print('Mean: {} us'.format(mean))
            median = stats.scoreatpercentile(sjrnTimes, 50)
            print('Median: {} us'.format(median))
            p95 = stats.scoreatpercentile(sjrnTimes, 95)
            print('95%: {} us'.format(p95))
            p99 = stats.scoreatpercentile(sjrnTimes, 99)
            print('99%: {} us'.format(p99))
            p999 = stats.scoreatpercentile(sjrnTimes, 99.9)
            print('99.9%: {} us'.format(p999))
            maxLat = max(sjrnTimes)
            print('Max Latency: {} us'.format(maxLat))
            minLat = min(sjrnTimes)
            print('Min Latency: {} us'.format(minLat))
        elif typeOfLats == '--slowdown':
            sjrnTimes = [l for l in latsObj.parseSojournTimes()]

            print('Num of Requests: {}'.format(len(sjrnTimes)))
            mean = np.mean(sjrnTimes)
            print('Mean: {} slowdown'.format(mean))
            median = stats.scoreatpercentile(sjrnTimes, 50)
            print('Median: {} slowdown'.format(median))
            p95 = stats.scoreatpercentile(sjrnTimes, 95)
            print('95%: {} slowdown'.format(p95))
            p99 = stats.scoreatpercentile(sjrnTimes, 99)
            print('99%: {} slowdown'.format(p99))
            p999 = stats.scoreatpercentile(sjrnTimes, 99.9)
            print('99.9%: {} slowdown'.format(p999))
            maxLat = max(sjrnTimes)
            print('Max Latency: {} slowdown'.format(maxLat))
        else:
            print(
                'Please use either "--latency" or "--slowdown" for the type of latency you want to calculate')
            exit(1)

    def calLatPct(typeOfLats, latsFile, tsFile):
        assert os.path.exists(latsFile)
        realtime_lat = []
        latsObj = Lat(latsFile)
        # print("======= {} ======".format(latsFile))
        tsObj = Lat(tsFile)
        # print("======= {} ======".format(tsFile))
        if typeOfLats == '--latency':
            # qTimes = [l/1e6 for l in latsObj.parseQueueTimes()]
            # svcTimes = [l/1e6 for l in latsObj.parseSvcTimes()]
            sjrnTimes = [l/1e3 for l in latsObj.parseSojournTimes()]

            # print('Num of Requests: {}'.format(len(sjrnTimes)))
            # convert to ms
            tsTimes = [l/1e6 for l in tsObj.parseSojournTimes()]
            start_time = tsTimes[0]
            second_lat_list = []
            # pdb.set_trace()
            for ts, lat in zip(tsTimes, sjrnTimes):
                if ts < start_time + 1e3:
                    second_lat_list.append(lat)
                else:
                    # the current second is over
                    p99 = stats.scoreatpercentile(second_lat_list, 99)
                    # print(round(p99,5))
                    realtime_lat.append(round(p99,5))
                    second_lat_list = []
                    start_time += 1e3
            print(realtime_lat)
        else:
            print(
                'Please use either "--latency" or "--slowdown" for the type of latency you want to calculate')
            exit(1)
    typeOfLats = sys.argv[1]
    latsFile = sys.argv[2]
    tsFile = sys.argv[3]
    calLatPct(typeOfLats, latsFile, tsFile)