# CarND-Controls-MPC
Self-Driving Car Engineer Nanodegree Program

---
## Introduction
This goal of this project is for navigate a track autonomously given a series of a waypoints on a track on the [udacity car simulator](https://github.com/udacity/self-driving-car-sim). A model predictive controller (MPC) is implemented using [Ipopt](https://projects.coin-or.org/Ipopt) and [CppAD](https://coin-or.github.io/CppAD/doc/cppad.htm) libraries.

## Implementation
### Overview
The inputs to the MPC are a series of waypoints, which are fitted with a 3rd degree polynomial and the current state. The optimiser then outputs an optimal control set (steering & accelerator) given a model, a set of constraints and a cost function. 

![](images/mpc_loop.png)

### Model
A kinematic bicycle model is used in this implementation which assumes a fixed speed and heading. The model consists of the state: x,y, psi (heading), v (speed), and errors: cross track error (cte), heading error. The state transition equations used the actuators (controls): delta (steering angle) and throttle (a).

![](images/model.png)

The model equations are implemented as constraints with both upper and lower limits set to zero.
  
Constraints were set on the actuation, steering was limited to [-25,25] degrees and throttle was limited to [-1,1] mpsps.

### Polynomial Fitting and MPC Preprocessing
To simplify the processing of waypoints they are first transformed into the vehicle frame such that the current state is always at the origin (x = 0, y = 0, psi = 0).

![](https://wikimedia.org/api/rest_v1/media/math/render/svg/04df6c10a7dccb95d6b6809825829f9187033a29)

```c++
            ptsx_vehicleframe(i) = (ptsx[i] - px) * cos(- psi) - (ptsy[i] - py) * sin(- psi);
            ptsy_vehicleframe(i) = (ptsx[i] - px) * sin(- psi) + (ptsy[i] - py) * cos(- psi);
```
### Model Predictive Control with Latency
The transformed state is now [0,0,0,v,cte,epsi] at time (t), however there's a delay of 100ms in the simulator. As such the controller's current state would not have been valid. To account for this delay the state after delay can be predicted using the state transition equations by using a dt = 0.1 sec. 

```c++
          state_predict(0) = 0.0 + v * dt;
          state_predict(1) = 0.0; 
          state_predict(2) = 0.0 + v * -delta / Lf * dt;
          state_predict(3) = v + a * dt;
          state_predict(4) = cte + v * sin(epsi) * dt;
          state_predict(5) = epsi + v * -delta / Lf * dt;
```
### Parameters
1- Timestep Length and Elapsed Duration (N & dt)


 N        | dt           | Comment  
 --- | --- | ---
 10     | 0.1 | Used value subjectivley improved tracking performance and stability 
 30      | 0.1      |   Slowed down the MPC without any tracking performance enhacements  
 10 | 0.2     |    Resulted in delay in the MPC and caused unpredictable tracking behavior  

2- Cost Function
Cost function and weights were the most critical part of the MPC implementation which affected the perfomance of the controller significantly.

The cost functions were used to follow the path closely (errors) and make the ride smooth (actuation cost)
Errors:
- Cross track error (CTE)
- Heading error (EPSI)
- Desired speed error (V - Vref)
Actuation cost:
- Steering cost
- Throttle cost
- Change in steering cost
- Change in throttle cost

Initially all cost functions were used with equal weights which resulted in instable behavior. The next step was adding weights to guide the MPC tracker. Based on intuition, change in steering cost and steering cost were assigned the largest weights to stablise the vehicle on the road. This resulted in reasonable tracking behavior however the vehicle never converged to the reference path as shown below.

![](images/Large_Error_Low_Delta.png)

The final weights used are shown below:

Cost function | Weight
 --- | --- 
Cross track error (CTE) | 100
Heading error (EPSI) | 100
Desired speed error (V - Vref) | 10
Steering cost | 1000
Throttle cost | 1000
Change in steering cost | 5000
Change in throttle cost | 10

## Performance Video
[![Performance Video](images/video.png)](https://www.youtube.com/watch?v=pFjRIFXmjkc)

## Dependencies

* cmake >= 3.5
 * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1(mac, linux), 3.81(Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools]((https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)
* [uWebSockets](https://github.com/uWebSockets/uWebSockets)
  * Run either `install-mac.sh` or `install-ubuntu.sh`.
  * If you install from source, checkout to commit `e94b6e1`, i.e.
    ```
    git clone https://github.com/uWebSockets/uWebSockets
    cd uWebSockets
    git checkout e94b6e1
    ```
    Some function signatures have changed in v0.14.x. See [this PR](https://github.com/udacity/CarND-MPC-Project/pull/3) for more details.

* **Ipopt and CppAD:** Please refer to [this document](https://github.com/udacity/CarND-MPC-Project/blob/master/install_Ipopt_CppAD.md) for installation instructions.
* [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page). This is already part of the repo so you shouldn't have to worry about it.
* Simulator. You can download these from the [releases tab](https://github.com/udacity/self-driving-car-sim/releases).
* Not a dependency but read the [DATA.md](./DATA.md) for a description of the data sent back from the simulator.


## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./mpc`.

## Tips

1. It's recommended to test the MPC on basic examples to see if your implementation behaves as desired. One possible example
is the vehicle starting offset of a straight line (reference). If the MPC implementation is correct, after some number of timesteps
(not too many) it should find and track the reference line.
2. The `lake_track_waypoints.csv` file has the waypoints of the lake track. You could use this to fit polynomials and points and see of how well your model tracks curve. NOTE: This file might be not completely in sync with the simulator so your solution should NOT depend on it.
3. For visualization this C++ [matplotlib wrapper](https://github.com/lava/matplotlib-cpp) could be helpful.)
4.  Tips for setting up your environment are available [here](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/23d376c7-0195-4276-bdf0-e02f1f3c665d)
5. **VM Latency:** Some students have reported differences in behavior using VM's ostensibly a result of latency.  Please let us know if issues arise as a result of a VM environment.

## Editor Settings

We've purposefully kept editor configuration files out of this repo in order to
keep it as simple and environment agnostic as possible. However, we recommend
using the following settings:

* indent using spaces
* set tab width to 2 spaces (keeps the matrices in source code aligned)

## Code Style

Please (do your best to) stick to [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).

## Project Instructions and Rubric

Note: regardless of the changes you make, your project must be buildable using
cmake and make!

More information is only accessible by people who are already enrolled in Term 2
of CarND. If you are enrolled, see [the project page](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/f1820894-8322-4bb3-81aa-b26b3c6dcbaf/lessons/b1ff3be0-c904-438e-aad3-2b5379f0e0c3/concepts/1a2255a0-e23c-44cf-8d41-39b8a3c8264a)
for instructions and the project rubric.

## Hints!

* You don't have to follow this directory structure, but if you do, your work
  will span all of the .cpp files here. Keep an eye out for TODOs.

## Call for IDE Profiles Pull Requests

Help your fellow students!

We decided to create Makefiles with cmake to keep this project as platform
agnostic as possible. Similarly, we omitted IDE profiles in order to we ensure
that students don't feel pressured to use one IDE or another.

However! I'd love to help people get up and running with their IDEs of choice.
If you've created a profile for an IDE that you think other students would
appreciate, we'd love to have you add the requisite profile files and
instructions to ide_profiles/. For example if you wanted to add a VS Code
profile, you'd add:

* /ide_profiles/vscode/.vscode
* /ide_profiles/vscode/README.md

The README should explain what the profile does, how to take advantage of it,
and how to install it.

Frankly, I've never been involved in a project with multiple IDE profiles
before. I believe the best way to handle this would be to keep them out of the
repo root to avoid clutter. My expectation is that most profiles will include
instructions to copy files to a new location to get picked up by the IDE, but
that's just a guess.

One last note here: regardless of the IDE used, every submitted project must
still be compilable with cmake and make./

## How to write a README
A well written README file can enhance your project and portfolio.  Develop your abilities to create professional README files by completing [this free course](https://www.udacity.com/course/writing-readmes--ud777).
