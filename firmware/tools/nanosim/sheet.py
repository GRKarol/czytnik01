import sys
from PIL import Image
names=sys.argv[2:]
ims=[Image.open('out/%s.ppm'%n).resize((1280,344),Image.NEAREST) for n in names]
sheet=Image.new('RGB',(1280,len(ims)*352),(80,80,80))
for i,im in enumerate(ims): sheet.paste(im,(0,i*352))
sheet.save('out/%s.png'%sys.argv[1])
