
export void saxpy_ispc(uniform int N,
                       uniform float scale,
                            uniform float X[],
                            uniform float Y[],
                            uniform float result[])
{
    foreach (i = 0 ... N) {           
        result[i] = scale * X[i] + Y[i];
    }
}

task void saxpy_ispc_task(uniform int N,
                               uniform int span,
                               uniform float scale,
                               uniform float* X, 
                               uniform float* Y,
                               uniform float* result_start)
{
    foreach (i = 0 ... span) {
        result_start[i] = scale * X[i] + Y[i];
    }
}

export void saxpy_ispc_withtasks(uniform int N,
                               uniform float scale,
                               uniform float X[],
                               uniform float Y[],
                               uniform float result[])
{

    uniform int span = N / 64;  // 64 tasks

    foreach (taskIndex = 0 ... 64) {
        uniform float *result_start = &result[taskIndex * span];
        uniform float *X_start = &X[taskIndex * span];
        uniform float *Y_start = &Y[taskIndex * span];
        launch[1] saxpy_ispc_task(N, span, scale, X_start, Y_start, result_start);
    }
}
