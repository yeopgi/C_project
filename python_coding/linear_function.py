
import numpy as np
import matplotlib.pylab as plt

# x range 설정
x = np.array(range(0, 10))

# 축 이름 설정
plt.xlabel('x axis')
plt.ylabel('y axis')

# 그리드 추가
plt.grid(color = "gray", alpha=.5, linestyle='--')

# 방정식 추가하기
plt.plot(x, 30*x + 10, label='y = 30*x + 10')

# 범례 작성
plt.legend()
plt.show()
