#ifndef CHECKGREAD_H_
#define CHECKGREAD_H_

#include "MyLib.h"
#include <Eigen/Dense>

using namespace Eigen;

class CheckGrad {

  public:
    vector<BaseParam*> _params;
    vector<string> _names;

  public:
    CheckGrad() {
        clear();
    }

    inline void clear() {
        _params.clear();
        _names.clear();
    }

    inline void add(BaseParam* param, const string& name) {
        _params.push_back(param);
        _names.push_back(name);
    }

  public:
    template<typename Example, typename Classifier>
    inline void check(Classifier* classifier, const vector<Example>& examples, const string& description) {
        dtype orginValue, lossAdd, lossPlus;
        int idx, idy;
        dtype mockGrad, computeGrad;
        for (int i = 0; i < _params.size(); i++) {
            _params[i]->randpoint(idx, idy);
			LDG::Tensor cpu_val;
			cpu_val.shape_ = _params[i]->val.shape();
			int size = cpu_val.shape().size();
			cpu_val.v = new dtype[size];
			cpu_val.device_type = CPU;	
			device.to_cpu(_params[i]->val, cpu_val);
			int row = cpu_val.shape().dims()[0];

            //orginValue = _params[i]->val[idx][idy];
            orginValue = cpu_val.v[idx * row + idy];


            //_params[i]->val[idx][idy] = orginValue + 0.001;
            cpu_val.v[idx * row + idy] = orginValue + 0.001;
			device.set(_params[i]->val, cpu_val.v, size);
            lossAdd = 0.0;
            for (int j = 0; j < examples.size(); j++) {
                lossAdd += classifier->cost(examples[j]);
            }

            //_params[i]->val[idx][idy] = orginValue - 0.001;

            cpu_val.v[idx * row + idy] = orginValue - 0.001;
			device.set(_params[i]->val, cpu_val.v, size);
            lossPlus = 0.0;
            for (int j = 0; j < examples.size(); j++) {
                lossPlus += classifier->cost(examples[j]);
            }
            mockGrad = (lossAdd - lossPlus) / 0.002;
            mockGrad = mockGrad / examples.size();
            //computeGrad = _params[i]->grad[idx][idy];
			LDG::Tensor cpu_grad;
			cpu_grad.shape_ = _params[i]->grad.shape();
			cpu_grad.v = new dtype[cpu_grad.shape().size()];
			cpu_grad.device_type = CPU;	

			device.to_cpu(_params[i]->grad, cpu_grad);

            computeGrad = cpu_grad.v[idx * row + idy];


            printf("%s, Checking gradient for %s[%d][%d]:\t", description.c_str(),
                   _names[i].c_str(), idx, idy);
            printf("mock grad = %.18f, computed grad = %.18f\n", mockGrad, computeGrad);

            cpu_val.v[idx * row + idy] = orginValue;
			device.set(_params[i]->val, cpu_val.v, size);
        }
    }

};



#endif /*CHECKGREAD_H_*/
