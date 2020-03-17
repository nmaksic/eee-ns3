#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# Author: Natasa Maksic, maksicn@etf.rs
#

import sys
import os
import numpy
from scipy.special import gamma as Gamma
from scipy.special import gammaincc as GammaIncc1
from scipy.misc import factorial as Factorial
from pylab import *
from scipy.stats import poisson
import math
from datetime import datetime

def GammaIncc(a, b):
	rez = (a, b)
	for y in range(a-1):
		rez = rez * (y + 1)
	return rez

def f3(x, lamda, Ex, C):
   ts = 0.00000288
   lts = lamda * ts
   Ce = 1.0 * C / Ex
   pjl = poisson.pmf(x, Ce)
   gm1 = GammaIncc1(x+2, lts) * (x+1)      
   gm2 = lts * GammaIncc1(x+1, lts)
   gm3 = lamda
   gm1a = gm1 / gm3
   gm2a = gm2 / gm3
   gammapart = gm1a - gm2a
   return pjl * gammapart

def fto(ts, tpp):
   rez = 0
   if tpp > ts:
      rez = tpp - ts
   return rez

def f32(x, lamda, Ex, C, to):
   ts = 0.00000288
   lts = lamda * ts

   lto = lamda * to + lamda * (fto(ts, 1/lamda)+ts)  #+ 1

   

   Ce = 1.0 * C / Ex
   pjl = poisson.pmf(x, Ce)

   gm1 = GammaIncc1(x+2, lts) * (x+1)      
   gm2 = lts * GammaIncc1(x+1, lts)
   gm3 = lamda
   gm1a = gm1 / gm3
   gm2a = gm2 / gm3

   gm1t = GammaIncc1(x+2, lto) * (x+1)      
   gm2t = lto * GammaIncc1(x+1, lto)
   gm3t = lamda
   gm1at = gm1t / gm3t
   gm2at = gm2t / gm3t

   gammapart = (gm1a - gm2a) - (gm1at - gm2at)

   return {'y0':pjl * gammapart, 'y1':pjl*(gm1a - gm2a) ,'y2':pjl*(gm1at - gm2at) }

def f3to(x, lamda, Ex, C, to):
   ts = 0.00000288
   lto = lamda * to + lamda * (fto(ts, 1/lamda)+ts) # + 1 # !!!
   Ce = 1.0 * C / Ex
   pjl = poisson.pmf(x, Ce)
   gm2 =  GammaIncc1(x+1, lto)
   return pjl * gm2


def calcsum(lamda):
     os.system("octave --silent --eval \"calcsum(" + str(lamda) + ")\"")
     with open("calcsum.res") as f:
         content = f.readlines()
         res = float(str.strip(content[3]))
         return res
     
def sumgamma(lamda, ro):
    rez = 0
    rez = calcsum(lamda)
    phioff = 0.1 
    ts = 0.00000288
    tw = 0.00000448
    result = 1-(1-phioff)*(1-ro)*rez/(rez + ts + tw)
    return result

def etoff2(lamda, Ex, C, Tto):
   rez = 0
   rez1 = 0
   rez2 = 0


   for x in range(1, 3500):
      result = f32(x, lamda, Ex, C, Tto)
      rez = rez + result['y0']
      rez1 = rez1 + result['y1']
      rez2 = rez2 + result['y2']
   
   Pto = 0
   
   
   for x in range(1, 3500):
      Pto = Pto + f3to(x, lamda, Ex, C, Tto)

   ts = 0.00000288
   return (1-Pto)*rez + Pto*(Tto+ fto(ts, 1/lamda) - ts) # pto


def pto(lamda, Ex, C, Tto):
   Pto = 0
   
   for x in range(0, 3500):
      Pto = Pto + f3to(x, lamda, Ex, C, Tto)

   return Pto

def sumgamma2(lamda, Ex, C, Tto, ro):
    rez = 0

    rez = etoff2(lamda, Ex, C, Tto)
    phioff = 0.1 

    ts = 0.00000288
    tw = 0.00000448
    result = 1-(1-phioff)*(1-ro)*rez/(rez + ts + tw) #
    return result

def eng_string( x, format='%s', si=True):
    sign = ''
    if x < 0:
        x = -x
        sign = '-'
    if x == 0:
        return "0"
    exp = int( math.floor( math.log10( x)))
    exp3 = exp - ( exp % 3)
    x3 = x / ( 10 ** exp3)

    if si and exp3 >= -24 and exp3 <= 24 and exp3 != 0:
        exp3_text = 'yzafpnum kMGTPEZY'[ ( exp3 - (-24)) / 3]
    elif exp3 == 0:
        exp3_text = ''
    else:
        exp3_text = 'e%s' % exp3

    return ( '%s'+format+'%s') % ( sign, x3, exp3_text)



def parse_file( fileName):
    f = open(fileName, 'r+')
    lines = f.readlines()	

    result = []
    for line in lines:
       a=[x.strip() for x in line.split(' ')]
       nodeId = int(a[0])
       portId = int(a[1])
       lpTimeNs = float(a[2])
       lpIntervals = int(a[3])
       packetCount = int(a[4])
       packetBytes = int(a[5])
       ex = float(a[6])
       linkspeed = float(a[7])
       theoreticEtoff = 0
       theoretic2 = 0
       if packetCount > 0:
          lamda = 1/ex
          Ex = packetBytes / packetCount
          mi = linkspeed / 8 / Ex 
          ro = 1.0 * lamda / mi
          Ts = 2.88e-9
          Tw = 4.48e-9
          phioff = 0.1
          EToff = 1e-9 * lpTimeNs / lpIntervals
          phi = 1-(1-phioff)*(1-ro)*(EToff / (EToff + Ts + Tw))
          if linkspeed == 10000000000:
             C = 24000
             offtimelimit = 0.0008
             theoreticEtoff = etoff2(lamda, int(round(float(Ex))), C, offtimelimit)
             theoretic2 = sumgamma2(lamda, int(round(float(Ex))), C, offtimelimit, ro)
          if linkspeed == 5000000000:
             C = 15000
             offtimelimit = 0.0008
             theoreticEtoff = etoff2(lamda, int(round(float(Ex))), C, offtimelimit)
             theoretic2 = sumgamma2(lamda, int(round(float(Ex))), C, offtimelimit, ro)
          print nodeId, portId, EToff, theoreticEtoff, phi, theoretic2
          result.append((nodeId, portId, EToff, theoreticEtoff, phi, theoretic2))
    return result



def parse_files(fileNameBase):
    result = []
    for i in range(1,101):
        result.extend(parse_file(fileNameBase + str(i) +  ".txt"))
    return result

def separatePortMeasurements(measurements):
    portMeasurements = {}
    for m in measurements:
        key = str(m[0]) + "_" + str(m[1]) # device and port
        portList = []
        if key in portMeasurements:
            portList = portMeasurements[key]
        else:
            portMeasurements[key] = portList
        portList.append(m)
    return portMeasurements

def calcStatistics(rndVariables):
    mean = numpy.mean(rndVariables)
    stdDev = numpy.std(rndVariables)
    z = 2.33 # 98%
    size = len(rndVariables)
    print z, stdDev, math.sqrt(size)
    marginOfError = z * stdDev / math.sqrt(size)
    return (mean, marginOfError)

def portCfdIntervals(measurements, f):
    simEtoff = []
    simEnergy = []
    thEtoff = []
    thEnergy = []
    device = -1
    port = -1
    for m in measurements:
        print "Measurement", m
        if device != -1 and device != m[0]:
            print "Error device"
        if port != -1 and port != m[1]:
            print "Error port"
        device = m[0]
        port = m[1]
        EToff = m[2]
        theoreticEtoff = m[3]
        phi = m[4]
        theoretic2 = m[5]
        simEtoff.append(EToff)
        simEnergy.append(phi)
        thEtoff.append(theoreticEtoff)
        thEnergy.append(theoretic2)
    statSimEtoff = calcStatistics(simEtoff)
    statTheoreticEtoff = calcStatistics(thEtoff)
    statPhi = calcStatistics(simEnergy)
    statTheoreticPhi = calcStatistics(thEnergy)
    f.write(str(device) + " " + str(port) + " " + str(statSimEtoff[0]) + " " + str(statSimEtoff[1]) + " " + str(statTheoreticEtoff[0]) + " " + str(statTheoreticEtoff[1]) + " " + str(statPhi[0]) + " " + str(statPhi[1]) + " " + str(statTheoreticPhi[0]) + " " + str(statTheoreticPhi[1]) + "\n")
    print device, port, statSimEtoff, statTheoreticEtoff, statPhi, statTheoreticPhi
    return

def calculateCfdIntervals(portMeasurements, resultFile):
    if os.path.exists(resultFile):
        os.remove(resultFile)
    f = open(resultFile, "a")
    for m in portMeasurements.items():
        portCfdIntervals(m[1], f)
    f.close()

measurements = parse_files("simulations/data")
portMeasurements = separatePortMeasurements(measurements)
calculateCfdIntervals(portMeasurements, "results.txt")

