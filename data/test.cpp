#include <iostream>
#include <climits>
using namespace std;

int main()
{
    float inf = 0;
    int *p = (int *)&inf;
    *p = *p | (0xfe << 23) | 0xf;
    float q = inf * 2;
    int *pp = (int *)&q;
    cout << inf << ' ' << q << endl;
}