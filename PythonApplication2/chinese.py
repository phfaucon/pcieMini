
def extended_gcd_a(a, b):
  assert a >= b and b >= 0 and a + b > 0

  if b == 0:
    d, x, y = a, 1, 0
  else:
        (p, q) = extended_gcd_a(b, a % b)
        x = q
        y = p - q * (a // b)

  return (x, y)
  
def ExtendedEuclid(a,b):
    if a < b:
      return extended_gcd_a(a, b%a)
    return extended_gcd_a(a,b)
  
def gcd(a, b):
  assert a >= 0 and b >= 0 and a + b > 0

  while a > 0 and b > 0:
    if a >= b:
      a = a % b
    else:
      b = b % a
  return max(a, b)

def diophantine(a, b, c):
  assert c % gcd(a, b) == 0
    
  # return (x, y) such that a * x + b * y = c
  # a * p + b * q = d
  # a * p * c2 + b * q * c2 = d * c2
  d = gcd(a,b)
  (p, q) = ExtendedEuclid(a, b)
#  print((d, p, q))
  c2 = c // d
  x =  p * c2
  y =  q * c2
#  print(x*a, y*b, c)
  return (x, y)
  
def ChineseRemainderTheorem(n1, r1, n2, r2):
    (x,y) = ExtendedEuclid(n1, n2)
    return (x * n1 * r2 + y * n2 * r1) % (n1 * n2)

def chinese_remainder_theorem(items):
    # Determine N, the product of all n_i
    N = 1
    for a, n in items:
        N *= n

    # Find the solution (mod N)
    result = 0
    for a, n in items:
        m = N // n
        r, s, d = extended_gcd(n, m)
        if d != 1:
            raise "Input not pairwise co-prime"
        result += a * s * m

    # Make sure we return the canonical solution.
    return result % N


def checkChineseRemainderTheorem(n1, r1, n2, r2):
    nberr = 0
    r = ChineseRemainderTheorem(n1, r1, n2, r2)
    prod = n1 * n2
    print("ChineseRemainderTheorem result = " + str(r))
    if (r < 2):
        print ("Failed: result is negative!")
        return
    if (r > prod):
        print ("Failed: result is greater than products!")
        return
    if (r % n1 != r1 ):
        print ("Failed: " + str(r) + " mod " + str(n1) + " = " + str(r % n1) + " != " + str(r1))
        nberr = 1
    else: 
        print ("Passed: " + str(r) + " mod " + str(n1) + " = " + str(r % n1) + " != " + str(r1))
    if (r % n2 != r2 ):
        print ("Failed: " + str(r) + " mod " + str(n2) + " = " + str(r % n2) + " != " + str(r2))
        return
    else: 
        print ("Passed: " + str(r) + " mod " + str(n2) + " = " + str(r % n2) + " != " + str(r2))
    if nberr == 0:
        print("Passed!")
        
def divide(a, b, n):
  assert n > 1 and a > 0 and gcd(a, n) == 1
  
  # ret    checkDiophantine(3,6,18)
def test():
    print (diophantine(24, 9, -6) )

    print (divide(4, 3, 7))

    print (gcd(1980, 1848))
    print (1980*1848/gcd(1980, 1848))

    checkDiophantine(686579304,26855093,1)
    checkChineseRemainderTheorem(686579304, 295310485, 26855093, 8217207)
    checkChineseRemainderTheorem(726600540, 585582861, 957970517, 93349856)
    #ChineseRemainderTheorem(686579304, 295310485, 26855093, 8217207) = -2305179499644022607538539, but must be between 0 and 686579304 * 26855093 = 18438151060795272
    #ChineseRemainderTheorem(726600540, 585582861, 957970517, 93349856) = 436145678203535241 mod 957970517 != 93349856 

def GCD(a, b):
  if b == 0:
    return a
  return GCD(b, a % b)
  
def ExtendedEuclid(a, b):
  if a < b:
    b = b%a
  if b == 0:
    d, x, y = a, 1, 0
  else:
        (p, q) = ExtendedEuclid(b, a % b)
        x = q
        y = p - q * (a // b)

  return (x, y)
  
def ChineseRemainderTheorem(n1, r1, n2, r2):
    (x,y) = ExtendedEuclid(n1, n2)
    return (x * n1 * r2 + y * n2 * r1) % (n1 * n2)

def DecipherCommonDivisor(first_ciphertext, first_modulo, first_exponent, second_ciphertext, second_modulo, second_exponent):
  # Fix this implementation to correctly decipher both messages in case
  # first_modulo and second_modulo share a prime factor, and return
  # a pair (first_message, second_message). The implementation below won't work
  # if the common_prime is bigger than 1000000.
  #for common_prime in range(2, 1000000):
  #  if first_modulo % common_prime == 0 and second_modulo % common_prime == 0:
  #    q1 = first_modulo // common_prime
  #    q2 = second_modulo // common_prime
      C = ChineseRemainderTheorem(first_ciphertext, first_modulo, second_ciphertext, second_modulo)
      M = IntSqrt(C)

      return (Decrypt(first_ciphertext, common_prime, q1, first_exponent), Decrypt(second_ciphertext, common_prime, q2, second_exponent))
  return ("unknown message 1", "unknown message 2")
