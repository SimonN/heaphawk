# heaphawk

A leightweight multi process memory tracker.

### Build:

Download or checkout the heaphawk source code and run cmake:
```
mkdir build
cd build
cmake ..
make
```
### Usage:

Start recording heap information about all processes that your user has access to:
```
./heaphawk record
```
Stop recording when you feel you have collected enough information by pressing ctrl+c. Then run

```
./heaphawk evaluate
```
to let heaphawk show you a list of the processes whose heap usage has increased. You can also type

```
./heaphawk plot
```
to let heaphawk create plotting files for gnuplot.





