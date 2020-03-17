rm -rf simulations
mkdir simulations
for i in {1..100}
do
  rm data.txt
  ./waf --command-template="%s --RngRun=$i" --run "scratch/leafspineppbp"
  cp data.txt simulations/data$i.txt
done

