import os

import random



class URLGenerator:

	"""

		Generate random urls until the url file increases to max_size

		@filename: path of the file for saving urls

		@max_num: the maximum number of distinct urls that will be generated

		@max_size: the maximum size of the file in bytes

	"""

	def generateURLs(self, filename, max_num, max_size):

		# First, generate max_num distinct URLs

		urls = set(); 

		while len(urls) < max_num:

			urls.add("http://www.url.com/" + ("%f" % random.random()) * random.choice(range(5)))

		urls = list(urls)

		# Second, fill the file using URLs chosen from the set above

		f = open(filename, 'w+')

		f.write('')

		f.close()

		f = open(filename, 'a')

		while (os.path.getsize(filename) < max_size):

			f.write(random.choice(urls) + '\n')

		f.close()

	

if __name__ == "__main__":

	byte = 1

	KB = 1000 * byte

	MB = 1000 * KB

	GB = 1000 * MB

	

	filename = "urls_s.txt"

	max_num = 1000 

	max_size = 10 * MB

	gen = URLGenerator()

	gen.generateURLs(filename, max_num, max_size)
	