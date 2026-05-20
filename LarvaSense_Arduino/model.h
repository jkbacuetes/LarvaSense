#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class SVM {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        float kernels[2] = { 0 };
                        float decisions[1] = { 0 };
                        int votes[2] = { 0 };
                        kernels[0] = compute_kernel(x,   12792.7  , 11885.0  , 13108.8  , 12252.8  , 11492.7  , 11403.3 );
                        kernels[1] = compute_kernel(x,   9985.6  , 9420.3  , 10000.0  , 10000.0  , 9427.0  , 9285.5 );
                        float decision = 8.691122466133;
                        decision = decision - ( + kernels[0] * -5.3409e-08 );
                        decision = decision - ( + kernels[1] * 5.3409e-08 );

                        return decision > 0 ? 0 : 1;
                    }

                    /**
                    * Predict readable class name
                    */
                    const char* predictLabel(float *x) {
                        return idxToLabel(predict(x));
                    }

                    /**
                    * Convert class idx to readable name
                    */
                    const char* idxToLabel(uint8_t classIdx) {
                        switch (classIdx) {
                            case 0:
                            return "UNINFESTED";
                            case 1:
                            return "INFESTED";
                            default:
                            return "Houston we have a problem";
                        }
                    }

                protected:
                    /**
                    * Compute kernel between feature vector and support vector.
                    * Kernel type: linear
                    */
                    float compute_kernel(float *x, ...) {
                        va_list w;
                        va_start(w, 6);
                        float kernel = 0.0;

                        for (uint16_t i = 0; i < 6; i++) {
                            kernel += x[i] * va_arg(w, double);
                        }

                        return kernel;
                    }
                };
            }
        }
    }