#!/usr/bin/python
# -*- coding: utf-8 -*-
import numpy as np
import matplotlib.pyplot as plt
from scipy.fftpack import fft
from scipy.signal import blackman
from numpy import hamming
from numpy import hanning
from plot import plot_old
import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("path", default='dane.txt')
args = parser.parse_args()

t=[]
dane=[]

with open(args.path, 'r') as csvfile:
	plots = csv.reader(csvfile, delimiter=',')
	for row in plots:
		try:
			t.append(int(row[0]))
			dane.append(float(row[1]))
		except:
			pass

#make x table start from zero	
t0 = t[0]
for i in range(len(t)):
	t[i] = (t[i]-t0)

#czestotliwosc sygnalu 100Hz
F=100
#ilosc probek
N=len(t)/2
#sredni okres probkowania
okres_ns= (t[-1])/float(len(t))
T=okres_ns/(10.0**9)
print(okres_ns, T)

x=np.linspace(0.0,T*N, N)
y = 1.5*np.sin(F*2.0*np.pi*x)
#generujemy szum na poziomie < 5us
noise = np.random.rand(N)/200000
x_noised = x + noise
y_noised = 1.5*np.sin(F*2.0*np.pi*x_noised)

#FFT
w = hanning(N)

yf_noised = fft(y_noised*w)
yf = fft(y*w)
xf = np.linspace(0.0, 1.0/(2.0*T), N/2)

fig, axes = plt.subplots(3,1)
axes[0].set_title(u'Sygnał idealny z rozrzutem czasu próbkowania')
axes[0].plot(x_noised[1:1000],y_noised[1:1000], marker='.')

axes[1].set_title(u'FFT sygnału idealnego z rozrzutem czasu próbkowania')
axes[1].semilogy(xf[1:N//2], 2.0/N * np.abs(yf_noised[1:N//2]), '-b')

axes[2].set_title(u'FFT sygnału idealnego')
axes[2].semilogy(xf[1:N//2], 2.0/N * np.abs(yf[1:N//2]), '-g')

fig.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)

# dla danych z przetwornika

dane = dane[:N]
t = t[:N]

fig, axes = plt.subplots(3,1)
axes[0].set_title(u'Sygnał spróbkowany z przetwornika MAX1202')
axes[0].plot(t[1:1000],dane[1:1000],'-r', marker='.')

dane_fourier = fft(dane*hanning(len(dane)))
tf = np.linspace(0.0, 1.0/(2.0*T), N/2)

axes[1].set_title(u'FFT sygnału spróbkowanego z przetwornika MAX1202')
#plt.plot(xf_noised, yf_noised)
axes[1].semilogy(tf[1:N//2], 2.0/N * np.abs(dane_fourier[1:N//2]), '-b')
axes[2].set_title(u'FFT sygnału idealnego')
axes[2].semilogy(xf[1:N//2], 2.0/N * np.abs(yf[1:N//2]), '-g')

fig.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)

#plt.savefig('sin_fft_max1202.png', bbox_inches='tight')

fig2 = plot_old(t, dane)
plt.show()
