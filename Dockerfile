FROM stagex/llvm:latest

RUN sudo apt update
RUN sudo apt install -y build-essential
RUN sudo apt install -y cmake