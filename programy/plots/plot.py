import matplotlib.pyplot as plt
import csv
import numpy as np
import math

def plot_old(x,y):
	averege_sampling_period = 0
	var = 0
	s = 0
	delta_x=[]
	samples = []
	variance = []

	#sampling_period	 
	averege_sampling_period = x[-1] / (len(x) - 1)

	#table of delta_x
	for i in range(len(x)-1):
		delta_x.append(x[i+1] - x[i])
		samples.append(i)

	#variance
	for i in range(len(x)-1):
		variance.append((delta_x[i]-averege_sampling_period)**2)
		var+=variance[i]

	var /= len(variance)

	#s
	s = math.sqrt(var)

	# Calculate the simple average of the data
	y_mean = [np.mean(y)]*len(x)


	print 'dlugosc bufora: ' + str(len(x))
	print 'sredni okres probkowania: ' + str(averege_sampling_period) + 'us'
	print 'srednia czestotliwosc probkowania: ' + str(1000000000/averege_sampling_period) + 'Hz'
	print 'wariancja okresu probkowania: ' + str(var)
	print 'odchylenie standardowe okresu probkowania: ' + str(s) + 'us'
	
	fig, ax = plt.subplots(1,1)
	ax.plot(samples,delta_x,'ro-', marker='.', label='Charakterystyka zmiennosci okresu probkowania')
	ax.set_xlabel('Numer probki')
	ax.set_ylabel('Okres probkowania dla danej probki [us]')
	ax.set_title('Charakterystyka zmiennosci okresu probkowania')
	fig.tight_layout()
	return fig
