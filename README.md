# Embedded Libs
This library is derived from the RIL layer in the openCPU framework, which has implemented a similar application on STM32 processors.

## Clone the repository
For clone repository with all submodules you have two choices:

#### 1. Clone the repository with submodules
```
git clone --recursive https://github.com/hamsadev/ril.git               # Version 1.65+
git clone --recursive -j8 https://github.com/hamsadev/ril.git           # Version 1.9+ 
git clone --recurse-submodules -j8 https://github.com/hamsadev/ril.git  # Version 2.13+
```
#### 2. Already cloned the repository 
```
git submodule update --init --recursive
```

#### 3. Pull all submodules
```
git pull --recurse-submodules
```
