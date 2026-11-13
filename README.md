# multi-threading
I am learning MT , so I thought to see how does concurrent processing by MT helps in speed and where it doesn't by some practice , the above code is matrix multiplication where we compare non-threaded and double threaded matrix multiplications , below is the output in seconds (implemented for two algorithms)
"""
1000
Populating the TWO Square matrices of Size 1000
Populating the matrices completed!!
NATIVE MM : 0.869339969
THREADS NATIVE MM : 0.401290135
TILE MM : 0.188316934
THREADS TILE MM : 0.150953454
"""

and there is an important aspect that we need to consider that is accesing from pointers slowed down my MT function as the program is loading the arrays everytime into memory instead of storing them in , because the compiler is constantly checking does this change the value in the pointer ,To the compiler, out and m are both just int*. It can’t prove they point to different arrays. Suppose someone called the function with out pointing one slot past m, so out[j] is the same memory as m[j+1]. Then:

iteration j writes to m[j+1]
iteration j+1 reads m[j+1], which now holds a new value

So each iteration depends on the previous one’s result, and they must run strictly in order.

Why that blocks vectorization

SIMD means loading 8 values of m at once, multiplying them, and storing 8 results at once. That is only correct if the loads don’t depend on the stores from earlier iterations. If out and m might overlap, doing 8 at a time could give a different answer than doing 1 at a time. So the compiler plays it safe and either emits scalar code, or adds a runtime “do these overlap?” check with a slow fallback path. so we changed that and it gave the expected result
