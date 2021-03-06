/*
 * Param.h
 *
 *  Created on: Jul 25, 2016
 *      Author: mason
 */

#ifndef PARAM_H_
#define PARAM_H_

#include "Eigen/Dense"
#include "BaseParam.h"



// Notice: aux is an auxiliary variable to help parameter updating
class Param : public BaseParam {
	public:
		LDG::Tensor aux_square;
		LDG::Tensor aux_mean;
		int iter;

		LDG::Tensor cpu_grad;

		LDG::Tensor v_r;
		LDG::Tensor grad_square;
		LDG::Tensor aux_eps;
		LDG::Tensor aux_sqrt;
		LDG::Tensor grad_alpha;
		LDG::Tensor grad_aux;

		LDG::Tensor belta_aux_mean;
		LDG::Tensor belta_grad;
		LDG::Tensor belta_aux_square;
		LDG::Tensor belta_grad_square;
		LDG::Tensor aux_square_eps;
		LDG::Tensor aux_square_eps_sqrt;
		LDG::Tensor aux_mean_lrt;
		LDG::Tensor val_delta;

		// allow sparse and dense parameters have different parameter initialization methods
		inline void initial(int outDim, int inDim) {
			//val.init(outDim, inDim);
			//grad.init(outDim, inDim);
			//aux_square.init(outDim, inDim);
			//aux_mean.init(outDim, inDim);
			//device.malloc(val, Shape({outDim, inDim}));
			device.init(grad, Shape({outDim, inDim}));
			device.init(aux_square, Shape({outDim, inDim}));
			device.init(aux_mean, Shape({outDim, inDim}));

			dtype bound = sqrt(6.0 / (outDim + inDim + 1));
			//val.random(bound);
			device.random_uniform(val, Shape({outDim, inDim}), -bound, bound);

			device.init(v_r, val.shape()); 
			device.init(grad_square, grad.shape());
			device.init(aux_eps, aux_square.shape());
			device.init(aux_sqrt, aux_square.shape());
			device.init(grad_alpha, grad.shape());
			device.init(grad_aux, grad.shape());

			device.init(belta_aux_mean, aux_mean.shape());
			device.init(belta_grad, grad.shape());
			device.init(belta_aux_square, aux_square.shape());
			device.init(belta_grad_square, grad.shape());            
			device.init(aux_square_eps, aux_square.shape());
			device.init(aux_square_eps_sqrt, aux_square.shape());
			device.init(aux_mean_lrt, aux_mean.shape());
			device.init(val_delta, val.shape());
			iter = 0;

			cpu_grad.device_type = CPU;	
			cpu_grad.shape_ = grad.shape();
			cpu_grad.v = new dtype[grad.shape().size()];
		}

		inline int outDim() {
			//return val.row;
			return val.shape().dims()[0];
		}

		inline int inDim() {
			//return val.col;
			return val.shape().dims()[1];
		}

		inline void clearGrad() {
			//grad.zero();
			device.zero(grad);
		}

		inline void updateAdagrad(dtype alpha, dtype reg, dtype eps) {
			if (outDim() > 1 && inDim() > 1) {
				device.Fmultiply_scalar(val, reg, v_r);
				device.Fadd(grad, v_r, grad);
				//grad.vec() = grad.vec() + val.vec() * reg;
			}
			device.Fsquare(grad, grad_square);
			device.Fadd(aux_square, grad_square, aux_square);
			//aux_square.vec() = aux_square.vec() + grad.vec().square();

			device.Fadd_scalar(aux_square, eps, aux_eps);
			device.Fsqrt(aux_eps, aux_sqrt);

			device.Fmultiply_scalar(grad, alpha, grad_alpha);

			device.Fdivide(grad_alpha, aux_sqrt, grad_aux);


			device.Fsubtract(val, grad_aux, val);
			//val.vec() = val.vec() - grad.vec() * alpha / (aux_square.vec() + eps).sqrt();
		}

		inline void updateAdam(dtype belta1, dtype belta2, dtype alpha, dtype reg, dtype eps) {
			if (outDim() > 1 && inDim() > 1) {
				device.Fmultiply_scalar(val, reg, v_r);
				device.Fadd(grad, v_r, grad);
			}
			device.Fmultiply_scalar(aux_mean, belta1, belta_aux_mean);

			device.Fmultiply_scalar(grad, 1 - belta1, belta_grad);

			device.Fadd(belta_aux_mean, belta_grad, aux_mean);
			device.Fmultiply_scalar(aux_square, belta2, belta_aux_square);


			device.Fsquare(grad, grad_square);

			device.Fmultiply_scalar(grad_square, (1 - belta2), belta_grad_square);

			device.Fadd(belta_aux_square, belta_grad_square, aux_square);
			dtype lr_t = alpha * sqrt(1 - pow(belta2, iter + 1)) / (1 - pow(belta1, iter + 1));

			device.Fadd_scalar(aux_square, eps, aux_square_eps);

			device.Fsqrt(aux_square_eps, aux_square_eps_sqrt);

			device.Fmultiply_scalar(aux_mean, lr_t, aux_mean_lrt);

			device.Fdivide(aux_mean_lrt, aux_square_eps_sqrt, val_delta);
			device.Fsubtract(val, val_delta, val);
			iter++;
			/*
			   if (val.col > 1 && val.row > 1)grad.vec() = grad.vec() + val.vec() * reg;
			   aux_mean.vec() = belta1 * aux_mean.vec() + (1 - belta1) * grad.vec();
			   aux_square.vec() = belta2 * aux_square.vec() + (1 - belta2) * grad.vec().square();
			   dtype lr_t = alpha * sqrt(1 - pow(belta2, iter + 1)) / (1 - pow(belta1, iter + 1));
			   val.vec() = val.vec() - aux_mean.vec() * lr_t / (aux_square.vec() + eps).sqrt();
			   iter++;
			 */
		}

		inline void randpoint(int& idx, int &idy) {
			//select indexes randomly
			std::vector<int> idRows, idCols;
			idRows.clear();
			idCols.clear();
			int dim0 = val.shape().dims()[0];
			int dim1 = val.shape().dims()[1];
			for (int i = 0; i < dim0; i++)
				idRows.push_back(i);
			for (int i = 0; i < dim1; i++)
				idCols.push_back(i);

			random_shuffle(idRows.begin(), idRows.end());
			random_shuffle(idCols.begin(), idCols.end());

			idy = idRows[0];
			idx = idCols[0];
		}

		inline dtype squareGradNorm() {
			dtype sumNorm = 0.0;

			device.to_cpu(grad, cpu_grad);
			int size = grad.shape().size();
			for (int i = 0; i < size; i++) {
				sumNorm += cpu_grad.v[i] * cpu_grad.v[i];
			}
			return sumNorm;
		}

		inline void rescaleGrad(dtype scale) {
			//grad.vec() = grad.vec() * scale;
			device.Fmultiply_scalar(grad, scale, grad);
		}

		inline void save(std::ofstream &os)const {
			/*
			   val.save(os);
			   aux_square.save(os);
			   aux_mean.save(os);
			   os << iter << endl;
			 */
		}

		inline void load(std::ifstream &is) {
			/*
			   val.load(is);
			   aux_square.load(is);
			   aux_mean.load(is);
			   is >> iter;
			 */
		}
};

#endif /* PARAM_H_ */
