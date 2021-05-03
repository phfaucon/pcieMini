def ModularExponentiation(b, k, m):
  r = b % m
  for _ in range(0,k):
    r = (r * r) % m
  return r

def FastModularExponentiation(b, e, m):
  
  # your code here
  remainder = b % m
  r = 1

  while e>0:
    if  e % 2 == 1:
      r = (r * remainder) % m
    remainder = (remainder * remainder) % m
    e = e//2
  return r

def Encrypt(message, modulo, exponent):
  # Fix this implementation
  return PowMod(ConvertToInt(message), exponent, modulo)

def Decrypt(ciphertext, p, q, exponent):
  d = InvertModulo(exponent,(p-1)*(q-1))
  return ConvertToStr(PowMod(ciphertext, d, p*q))
  
a = 3
b = 7
c = InvertModulo(a, b)
print(c)

p = 1000000007
q = 1000000009
exponent = 23917
modulo = p * q
ciphertext = Encrypt("attack", modulo, exponent)
message = Decrypt(ciphertext, p, q, exponent)
print(message)

print(ModularExponentiation(45567891223, 7, 11))
print(FastModularExponentiation(45567891223, 128, 11))

def DecipherSimple(ciphertext, modulo, exponent, potential_messages):
  # Fix this implementation
  for j in range(len(potential_messages)):
    if ciphertext == Encrypt(potential_messages[j], modulo, exponent):
      return potential_messages[j]
  return "don't know"

modulo = 101
exponent = 12
ciphertext = Encrypt("attack", modulo, exponent)
print(ciphertext)
print(DecipherSimple(ciphertext, modulo, exponent, ["attack", "don't attack", "wait"]))
ciphertext = Encrypt("don't attack", modulo, exponent)
print(ciphertext)
print(DecipherSimple(ciphertext, modulo, exponent, ["attack", "don't attack", "wait"]))

def isqrt(n):
    x = n
    y = (x + 1) // 2
    while y < x:
        x = y
        y = (x + n // x) // 2
    return x
    

def DecipherSmallPrime(ciphertext, modulo, exponent):
  if modulo % 2 == 0:
    small_prime = 2
    big_prime = modulo // 2
    return Decrypt(ciphertext, small_prime, big_prime, exponent)
  t = 3
  for i in range(t,t+10000000,2):
    if modulo % i == 0:
      print (modulo//i)
      return Decrypt(ciphertext, modulo//i, i, exponent)
  return "don't know"

def GCD(a, b):
  if b == 0:
    return a
  return GCD(b, a % b)
  
def DecipherCommonDivisor(first_ciphertext, first_modulo, first_exponent, second_ciphertext, second_modulo, second_exponent):
  # Fix this implementation to correctly decipher both messages in case
  # first_modulo and second_modulo share a prime factor, and return
  # a pair (first_message, second_message). The implementation below won't work
  # if the common_prime is bigger than 1000000.
  common_prime = GCD(first_modulo, second_modulo)
  if first_modulo % common_prime == 0 and second_modulo % common_prime == 0:
    q1 = first_modulo // common_prime
    q2 = second_modulo // common_prime
    return (Decrypt(first_ciphertext, common_prime, q1, first_exponent), Decrypt(second_ciphertext, common_prime, q2, second_exponent))
  return ("unknown message 1", "unknown message 2")
  
# Example usage with common prime p and different second primes q1 and q2  
p = 101
q1 = 18298970732541109011012304219376080251334480295537316123696052970419466495220522723330315111017831737980079504337868198011077274303193766040393009648852841770668239779097280026631944319501437547002412556176186750790476901358334138818777298389724049250700606462316428106882097210008142941838672676714188593227684360287806974345181893018133710957167334490627178666071809992955566020058374505477745993383434501768887090900283569055646901291270870833498474402084748161755197005050874785474707550376333429671113753137201128897550014524209754619355308207537703754006699795711188492048286436285518105948050401762394690148387
q2 = 1000000007
first_modulo = p * q1
second_modulo = p * q2
first_exponent = 239
second_exponent = 17
first_ciphertext = Encrypt("attack", first_modulo, first_exponent)
second_ciphertext = Encrypt("wait", second_modulo, second_exponent)
print(DecipherCommonDivisor(first_ciphertext, first_modulo, first_exponent, second_ciphertext, second_modulo, second_exponent))