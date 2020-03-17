
# Energy Efficient Ethernet with Byte Based Coalescing - Implementation of ns-3 Network Device

This repository contains implementation of ns3 net device which supports Energy Efficient Ethernet with coalescing. The implemlementation is developed in ns-3.30.1.  Installation procedure is the following:

- copy folder point-to-point coalescing to ns3 src folder
- confugure ns3 and buld EEE net device using commands ./waf configure, and ./waf build
 
Associated example file requires uses PPBP application which can be obtained from https://github.com/sharan-naribole/PPBP-ns3. In order to execute test:

- copy example file example/leafspineppbp.cc to ns3 scratch folder
- execute command: ./waf --run "scratch/leafspineppbp"

Additional scripts for running and processing set of simulations are available. Bash script simulations.sh should be copied to ns3 folder and executed without arguments. It will execute 100 simulations and store results in folder simulations. Python script calculate.py will use the results from the folder simulations and calculate confidence intervals for mean duration of low-power state E[Toff] and ratio of energy consumption with and without EEE. Script should be located in the folder which contain folder simulations. It can be executed using command: python calculate.py . Script calculate.py will store its results in file results.txt.




