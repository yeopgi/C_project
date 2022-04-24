def get_gcm(src_1, src_2):
    gcm = 1
    for i in range(2, min(src_1, src_2) + 1):
        while (src_1 % i == 0) & (src_2 % i == 0):
            src_1 /= i
            src_2 /= i
            gcm *= i

    return gcm

print(get_gcm(18, 6))