#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;

// Set the timestep length and duration
size_t N = 10;
double dt = 0.1;

size_t x_start = 0;
size_t y_start = x_start + N;
size_t psi_start = y_start + N;
size_t v_start = psi_start + N;
size_t cte_start = v_start + N;
size_t epsi_start = cte_start + N;
size_t delta_start = epsi_start + N;
size_t a_start = delta_start + N - 1;
// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
// simulator around in a circle with a constant steering angle and velocity on a
// flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;

class FG_eval {
 public:
  // Fitted polynomial coefficients
  Eigen::VectorXd coeffs;
  FG_eval(Eigen::VectorXd coeffs) { this->coeffs = coeffs; }

  typedef CPPAD_TESTVECTOR(AD<double>) ADvector;
  void operator()(ADvector& fg, const ADvector& vars) {
    
    // The cost is stored is the first element of `fg`.
    // Any additions to the cost should be added to `fg[0]`.
    fg[0] = 0;
    // cost function

    
    for (size_t t = 0; t < N; t++) {
      fg[0] += 100 * CppAD::pow(vars[cte_start + t], 2);
      fg[0] += 100 * CppAD::pow(vars[epsi_start + t], 2);
      fg[0] += 10 * CppAD::pow(vars[v_start + t] - 40, 2);

    }
    
    for (size_t t = 0; t < N-1; t++) {
      fg[0] += 1000 * CppAD::pow(vars[delta_start + t], 2);
      fg[0] += 1000 * CppAD::pow(vars[a_start + t], 2);
      // lateral acceleration
      // fg[0] += 5 * CppAD::pow(vars[v_start + t] * vars[v_start + t] * vars[delta_start + t]/Lf, 2);

    }
    
    for (size_t t = 0; t < N-2; t++) {
      fg[0] += 5000 * CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2);
      fg[0] += 10 * CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);
    }


    
    // Setup Model Constraints
    
    // Initial constraints
    // We add 1 to each of the starting indices due to cost being located at index 0 of `fg`.
    // This bumps up the position of all the other values.
    fg[1 + x_start] = vars[x_start];
    fg[1 + y_start] = vars[y_start];
    fg[1 + psi_start] = vars[psi_start];
    fg[1 + v_start] = vars[v_start];
    fg[1 + cte_start] = vars[cte_start];
    fg[1 + epsi_start] = vars[epsi_start];
    
    // The rest of the constraints
    for (size_t t = 1; t < N; t++) {
      // State at time t + 1
      AD<double> x1 = vars[x_start + t];
      AD<double> y1 = vars[y_start + t];
      AD<double> psi1 = vars[psi_start + t];
      AD<double> v1 = vars[v_start + t];
      AD<double> cte1 = vars[cte_start + t];
      AD<double> epsi1 = vars[epsi_start + t];
      
      // State at time t
      AD<double> x0 = vars[x_start + t - 1];
      AD<double> y0 = vars[y_start + t - 1];
      AD<double> psi0 = vars[psi_start + t - 1];
      AD<double> v0 = vars[v_start + t - 1];
      AD<double> cte0 = vars[cte_start + t - 1];
      AD<double> epsi0 = vars[epsi_start + t - 1];
      
      // Actuator constraints at time t only
      AD<double> delta0 = vars[delta_start + t - 1];
      AD<double> a0 = vars[a_start + t - 1];
      
      AD<double> f0 = coeffs[0] + coeffs[1] * x0 + coeffs[2] * pow(x0, 2) + coeffs[3] * pow(x0, 3);
      AD<double> psi_des0 = CppAD::atan(coeffs[1] + 2*coeffs[2]*x0 + 3*coeffs[3]*pow(x0,2));
      
      // Setting up the rest of the model constraints
      fg[1 + x_start + t] = x1 - (x0 + v0 * CppAD::cos(psi0) * dt);
      fg[1 + y_start + t] = y1 - (y0 + v0 * CppAD::sin(psi0) * dt);
      fg[1 + psi_start + t] = psi1 - (psi0 - v0 * delta0 / Lf * dt);
      fg[1 + v_start + t] = v1 - (v0 + a0 * dt);
      fg[1 + cte_start + t] = cte1 - ((f0-y0) + (v0 * CppAD::sin(epsi0) * dt));
      fg[1 + epsi_start + t] = epsi1 - ((psi0 - psi_des0) - v0 * delta0 / Lf * dt);
    }
  }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}
MPC::~MPC() {}

vector<double> MPC::Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs) {
  bool ok = true;
  typedef CPPAD_TESTVECTOR(double) Dvector;

  size_t N_state = 6;
  size_t N_controls = 2;
  size_t n_vars = N_state * N + N_controls * (N - 1);
  size_t n_constraints =  N * N_state; // state transition + controls errors

  // Initial value of the independent variables.
  Dvector vars(n_vars);
  for (size_t i = 0; i < n_vars; i++) {
    vars[i] = 0;
  }

  vars[x_start]       = state[0]; // x
  vars[y_start]       = state[1]; // y
  vars[psi_start]     = state[2]; // heading
  vars[v_start]       = state[3]; // speed
  vars[cte_start]     = state[4]; // cte
  vars[epsi_start]    = state[5]; // throttle

  Dvector vars_lowerbound(n_vars);
  Dvector vars_upperbound(n_vars);

  for (size_t i = 0; i < delta_start; i++) {
    vars_lowerbound[i] = -1.0e19;
    vars_upperbound[i] = 1.0e19;
  }
  
  // Set lower and upper limits for steering
  for (size_t i = delta_start; i < a_start - 1; i++) {
    vars_lowerbound[i] = -M_PI*25/180.0;
    vars_upperbound[i] = M_PI*25/180.0;
  }
  // Set lower and upper limits for throttle
  for (size_t i = a_start; i < n_vars; i++) {
    vars_lowerbound[i] = -1.0f;
    vars_upperbound[i] = 1.0f;
  }

  // Lower and upper limits for the constraints
  Dvector constraints_lowerbound(n_constraints);
  Dvector constraints_upperbound(n_constraints);
  for (size_t i = 0; i < n_constraints; i++) {
    constraints_lowerbound[i] = 0;
    constraints_upperbound[i] = 0;
  }

  constraints_lowerbound[x_start] = state[0];
  constraints_lowerbound[y_start] = state[1];
  constraints_lowerbound[psi_start] = state[2];
  constraints_lowerbound[v_start] = state[3];
  constraints_lowerbound[cte_start] = state[4];
  constraints_lowerbound[epsi_start] = state[5];

  constraints_upperbound[x_start] = state[0];
  constraints_upperbound[y_start] = state[1];
  constraints_upperbound[psi_start] = state[2];
  constraints_upperbound[v_start] = state[3];
  constraints_upperbound[cte_start] = state[4];
  constraints_upperbound[epsi_start] = state[5];

  // object that computes objective and constraints
  FG_eval fg_eval(coeffs);

  // options for IPOPT solver
  std::string options;
  // Uncomment this if you'd like more print information
  options += "Integer print_level  0\n";
  // NOTE: Setting sparse to true allows the solver to take advantage
  // of sparse routines, this makes the computation MUCH FASTER. If you
  // can uncomment 1 of these and see if it makes a difference or not but
  // if you uncomment both the computation time should go up in orders of
  // magnitude.
  options += "Sparse  true        forward\n";
  options += "Sparse  true        reverse\n";
  // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
  // Change this as you see fit.
  options += "Numeric max_cpu_time          0.5\n";

  // place to return solution
  CppAD::ipopt::solve_result<Dvector> solution;

  // solve the problem
  CppAD::ipopt::solve<Dvector, FG_eval>(
      options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
      constraints_upperbound, fg_eval, solution);

  // Check some of the solution values
  ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

  // Cost
  auto cost = solution.obj_value;
  std::cout << "Cost " << cost << std::endl;

  std::vector<double> sol;

  sol.push_back(solution.x[delta_start]);
  sol.push_back(solution.x[a_start]);

  std::cout << "solution.x[delta_start]:" << solution.x[delta_start] << std::endl;
  std::cout << "solution.x[a_start]:" << solution.x[a_start] << std::endl;

  for (size_t i = 0; i < N-1; i++) {
    sol.push_back(solution.x[x_start + i + 1]);
    sol.push_back(solution.x[y_start + i + 1]);
  }

  return sol;
}
