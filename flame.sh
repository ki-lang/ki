
# cargo install flamegraph
# sudo bash -c 'vim ~/.bashrc'
# PATH=$PATH:~/.cargo/bin

mkdir -p flame
#sudo bash -i -c 'cd flame && flamegraph -o graph.svg ../test'
sudo bash -i -c 'cd flame && flamegraph -o graph.svg -p 1602207'
