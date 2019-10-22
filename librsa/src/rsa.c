#include <rsa.h>
#include <libalpaca/alpaca.h>
#include <libio/console.h>
#include <math.h>




int16_t gcd(int16_t a, int16_t b)
{
    int16_t q, r1, r2, r;

    // while(n1!=n2)
    // {
    //     if(n1 > n2)
    //         n1 -= n2;
    //     else
    //         n2 -= n1;
    // }

    if(a> b)
    {
        r1 = a;
        r2 = b;
    }
    else
    {
        r1 = b;
        r2 = a;
    }

    while (r2 > 0)
    { 
        q = r1 / r1;
        r = r1 - q * r2;
        r1 = r2;
        r2 = r;
    }
    
    return r1;
}

void fast_exponentiation(int16_t bit, int16_t n, int16_t* y, int16_t* a)
{
    if (bit == 1)
    {
        *y = (*y * (*a)) % n;
    }
    *a = (*a) * (*a) % n;
}

int16_t find_T(int16_t a, int16_t m, int16_t n)
{
    int16_t r;
    int16_t y = 1;
    while (m > 0)
    {
        r = m % 2;
        fast_exponentiation(r, n, &y, &a);
        m = m / 2;
    }
    return y;
}

int16_t primary_test(int16_t a, int16_t i)
{
    int16_t n = i-1;
    int16_t k = 0;
    int16_t j, m, T;

    while (n % 2 == 0)
    {
        k++;
        n = n / 2; 
    }

    m = n;
    T = find_T(a, m, i);

    if( T == 1 || T == i - 1)
    {
        return 1;
    }

    for (j = 0; j < k; j++)
    {
        T = find_T(T, 2, i);
        if (T == 1)
        {
            return 0;
        }
        if(T == i - 1)
        {
            return 1;
        }
    }
    return 0;
}

int16_t inverse(int16_t a, int16_t b)
{
	int16_t inv = 0;
	int16_t q, r, r1 = a, r2 = b, t, t1 = 0, t2 = 1;

	while (r2 > 0) {
		q = r1 / r2;
		r = r1 - q * r2;
		r1 = r2;
		r2 = r;

		t = t1 - q * t2;
		t1 = t2;
		t2 = t;
	}

	if (r1 == 1)
		inv = t1;

	if (inv < 0)
		inv = inv + a;

	return inv;
}

// int16_t key_generation()
// {
//     int16_t p, q;
//     int16_t phi_n;
//     int16_t e, d, n;
//     int16_t temp;

//     while(1)
//     {
//         p = rand();
//         temp = primary_test(2, p);
//         if(temp == 1)
//         {
//             break;
//         }
//     }
    
//     do
//     {
//         do
//         {
//            q = rand();
//         } while (q % 2 == 0);
        
//     } while (!primary_test(2, q));
    
//     n = p * q;
//     phi_n = (p-1) * (q-1);

//     do
//     {
//         e = rand() % (phi_n - 2) + 2;
//     } while (gcd(e, phi_n) != 1);

//     d = inverse(phi_n, e);
    
// }

int16_t key_generation1()
{
    int16_t p;
    do {
		do
			p = rand();
		while (p % 2 == 0);

	} while (!primary_test(2, p));
    return p;
}

int16_t key_generation2()
{
    int16_t q;
    do
    {
        do
        {
           q = rand();
        } while (q % 2 == 0);
        
    } while (!primary_test(2, q));
    return q;

}

void key_generation3(int16_t p, int16_t q, int16_t *ed)
{
    int16_t n, phi_n, e;
    n = p * q;
    phi_n = (p-1) * (q-1);

    e = 2;
    while(e <phi_n)
    {
        if(gcd(e, phi_n)==1)
        {
            break;
        }
        else
        {
            e++;
        }
        
    }

    
    ed[0] = e;
    ed[2] = phi_n;
    ed[3] = n;
}

void key_generation4(int16_t *ed)
{
    ed[1] = inverse(ed[2], ed[0]);
}

