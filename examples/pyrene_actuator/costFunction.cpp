#include "costFunction.hh"
#include <iostream>
#include <math.h>
#include <cmath>     

using namespace std;   

const double CostFunctionPyreneActuator::K = 10.6; 

CostFunctionPyreneActuator::CostFunctionPyreneActuator()
{
    Q << 10.0, 0.0,
         0.0, 0.01; 
    W << 1.0, 0.0,
         0.0, 0.01; 
    R << 0.0001;
    P << 1.0;

    lxx = Q;
    luu = R;
    lux << 0.0,0.0;
    lxu << 0.0,0.0;
    lx.setZero();
    final_cost = 0.0;
    running_cost = 0.0;
    tauLim = 0.0;
    lambdaLim = 10.0;
    alphaTau = 0.01;
}

void CostFunctionPyreneActuator::setTauLimit(double limit)
{
    tauLim = limit;
}

void CostFunctionPyreneActuator::setJointLimit(double limitUp, double limitDown)
{
    jointLim.push_back(limitUp);
    jointLim.push_back(limitDown);
}

void CostFunctionPyreneActuator::setJointVelLimit(double limitUp, double limitDown)
{
    jointVelLim.push_back(limitUp);
    jointVelLim.push_back(limitDown);
}

void CostFunctionPyreneActuator::computeTauConstraintsAndDeriv(const commandVec_t& U)
{
    double maxTau = 1 - alphaTau * (tauLim - K*U[0]);
    double minTau = 1 - alphaTau * (K*U[0] + tauLim);   
    TauConstraints << exp(alphaTau * maxTau) + exp(alphaTau * minTau);
    dTauConstraints << alphaTau*alphaTau * K * (exp(alphaTau * maxTau) - exp(alphaTau * minTau));
    ddTauConstraints << pow(alphaTau, 4.0) * K*K * (exp(alphaTau * maxTau) - exp(alphaTau * minTau));
}

void CostFunctionPyreneActuator::computeConstraintsAndDeriv(const stateVec_t& X)
{
    double maxJoint = 1 - lambdaLim * (jointLim[0] - X[0]);
    double minJoint = 1 - lambdaLim * (X[0] - jointLim[1]);    
    Constraints[0] = exp(lambdaLim * maxJoint) + exp(lambdaLim * minJoint);

    double maxJointVel = 1 - lambdaLim * (jointVelLim[0] - X[0]);
    double minJointVel = 1 - lambdaLim * (X[0] - jointVelLim[1]);    
    Constraints[1] = exp(lambdaLim * maxJointVel) + exp(lambdaLim * minJointVel);

    double d0 = lambdaLim*lambdaLim * (exp(lambdaLim * maxJoint) - exp(lambdaLim * minJoint));
    double d1 = lambdaLim*lambdaLim * (exp(lambdaLim * maxJointVel) - exp(lambdaLim * minJointVel));
    dConstraints << d0, 0.0, 
                    0.0, d1; 

    double dd0 = pow(lambdaLim, 4.0) * Constraints[0];
    double dd1 = pow(lambdaLim, 4.0) * Constraints[1];
    ddConstraints << dd0, 0.0, 
                     0.0, dd1;               
}

void CostFunctionPyreneActuator::computeCostAndDeriv(const stateVec_t& X,const stateVec_t& Xdes, const commandVec_t& U)
{    
    computeConstraintsAndDeriv(X);
    computeTauConstraintsAndDeriv(U);
    running_cost =  ((X - Xdes).transpose() * Q * (X - Xdes) + U.transpose() * R * U + Constraints.transpose() * W * Constraints \
                    + TauConstraints.transpose() * P * TauConstraints)(0, 0);    

    lx = Q*(X-Xdes) + (2.0*dConstraints.transpose()*W*Constraints);
    Eigen::Matrix<double, 2, 1> tempDD;
    tempDD = ddConstraints*W*Constraints; 
    Eigen::Matrix<double, 2, 2> dd;
    dd << tempDD[0], 0.0,
          0.0, tempDD[1];
    lxx = Q + (2.0*(dConstraints.transpose()*W*dConstraints + dd));
    lu = R*U + (2.0*dTauConstraints.transpose()*P*TauConstraints);
    luu = R + (2.0*(ddTauConstraints.transpose()*P*TauConstraints + dTauConstraints.transpose()*P*dTauConstraints));
}

void CostFunctionPyreneActuator::computeFinalCostAndDeriv(const stateVec_t& X,const stateVec_t& Xdes)
{
    computeConstraintsAndDeriv(X);
    lx = Q*(X-Xdes) + (2.0*dConstraints.transpose()*W*Constraints);
    Eigen::Matrix<double, 2, 1> tempDD;
    tempDD = ddConstraints*W*Constraints; 
    Eigen::Matrix<double, 2, 2> dd;
    dd << tempDD[0], 0.0,
          0.0, tempDD[1];
    lxx = Q + (2.0*(dConstraints.transpose()*W*dConstraints + dd));
}
