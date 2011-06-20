#!/usr/bin/python
import sys, os
import math

import matplotlib
print "Using MPL version:", matplotlib.__version__

import pylab
from pylab import *


def graph(log_file, save=True):
    '''Process the log files generated by srvctcp stored in the logs/ directory.'''
    times      = []
    instant_BW = []
    average_BW = []
    blockno    = []
    snd_cwnd   = []
    ssthresh    = []
    slr        = []
    slr_long   = []
    srtt       = []
    rto        = []
    rtt        = []

    current = 0.1
    mss     = 1398
    res     = 0.01
    abw     = 0
    bw      = 0

    print "Processing log at: " + log_file

    f = open(log_file, 'r')
    for line in f:
        values = line.split(" ")

        if values[-1] == "xmt\n":
            # The variable values is a list of the form
            # [time, blockno, snd_cwnd, slr, rtt, srtt, rto, xmt]

            # Compute instant and avg bw

            abw += mss
            bw  += mss

            time = float(values[0])

            if time > current and time <= (current + res):
                times.append(time)

                instant_BW.append(8e-6*bw/res)
                average_BW.append(8e-6*1.0*abw/current)

                blockno.append(values[1])
                snd_cwnd.append(values[2])
                ssthresh.append(values[3])
                slr.append(values[4])
                slr_long.append(values[5])
                srtt.append(values[6])
                rto.append(values[7])
                rtt.append(values[8])


                current = res + current
                bw = 0
            elif time > (current + res):
                zero_point = res*math.floor(time/res)
                times.append(current)
                times.append(zero_point)

                instant_BW.append(0)
                instant_BW.append(0)

                average_BW.append(8e-6*1.0*abw/current)
                average_BW.append(8e-6*1.0*abw/zero_point)

                blockno.append(values[1])
                blockno.append(values[1])

		# need to appen twice to account for zero_point
                snd_cwnd.append(values[2])
                snd_cwnd.append(values[2])

                ssthresh.append(values[3])
                ssthresh.append(values[3])
                slr.append(values[4])
                slr.append(values[4])
                slr_long.append(values[5])
                slr_long.append(values[5])
                srtt.append(values[6])
                srtt.append(values[6])
                rto.append(values[7])
                rto.append(values[7])
                rtt.append(values[8])
                rtt.append(values[8])



		current = zero_point + res
		bw = 0

    # print average_BW
    subplot(411)
    plot(times, instant_BW, 'r-', times, average_BW, 'g-')
    grid(True)
    title('CTCP Performance')
    ylabel('Mbs')

    # print congestion window
    subplot(412)
    plot(times, snd_cwnd, 'b*', times, ssthresh, 'r-')
    grid(True)
    ylabel('Congestion window (packets)')

    # print round trip time and rto
    subplot(413)
    plot(times, rtt, 'b-', times, srtt, 'r-')
    grid(True)
    ylabel('time (s)')

    # print loss rate
    subplot(414)
    plot(times, slr, 'r-', times, slr_long, 'b-')
    grid(True)
    xlabel('time (s)')
    ylabel('loss rate')

    F = pylab.gcf()


    fig_path = log_file.replace('logs', 'figs')
    fig_path = fig_path.replace('log', 'pdf')

    if save:
        if not os.path.isabs(fig_path):
            fig_dir = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(__file__)), '..', os.path.dirname(fig_path)))

            if not os.path.exists(fig_dir):
                os.makedirs(fig_dir)

        DefaultSize = F.get_size_inches()
        F.set_size_inches( (DefaultSize[0]*2, DefaultSize[1]*2) )

        F.savefig(fig_path, orientation='portrait', papertype='letter')
    else:
        show()


def main(args):
    graph(args[0], save=True)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit("Usage: ./scripts/grapher <srvctcp_output>")
    main(sys.argv[1:])


