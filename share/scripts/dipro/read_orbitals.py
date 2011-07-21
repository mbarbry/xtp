def readOrb(path,whichMonomer,nsaosOfOtherMolecule):
  """-"path" specifies the location of the orbital file
  -"whichMonomer" (either 1 or 2) specifies where to put the zeros into the orbital file
  -"nsaosOfOtherMolecule" number of zeros to be put as placeholders for the other molecule
  -the function returns the field vec_mo
  """


  c_mo=[]
  c_fill_mo=[]
  vec_mo=[]
  LastLine=[]


  if whichMonomer!=1 and whichMonomer!=2:
     print "whichMonomer has to be either 1 or 2!"
     sys.exit()
  

  moFile=open(path,'r')
  readMo='false'
  for line in moFile:
	  #skip comment lines
          if line.find('#') == 0:
	    continue

	  #stop reading when $end statement is reached
   	  if '$end' in line:
     	    break

	  #analyse mo-file header
	  if 'scfconv' in line:
	    cols=line.split('=')
	    str0=cols[1].split(' ')
	    scfconv=int(str0[0])
	  line = line.strip()

	  #read eigenvalue and calculate size of mo-block
	  if 'nsaos' in line:
	      lineCounter=1
	      readMo='true'
      	      cols=line.split('=')
	      nsaos=int(cols[2])
	      str1=cols[1].split(' ')
	      eigenvalue=[float(str1[0].replace('D','e'))]
	      ElementsInLastLine=nsaos%4
	      if ElementsInLastLine != 0:
 		  NumberOfLines=(nsaos/4+1)
	      else:
		  NumberOfLines=(nsaos/4)
	      continue

  	  #read mo-coefficients    
	  if readMo == 'true':     
  	    CoeffString=line
  	    #put the mo-coefficients into c_mo1 
  	    if lineCounter < NumberOfLines:
  	      for j in range(4):
  		c_mo.append(  CoeffString[0+j*20:20+j*20] )
  	      lineCounter+=1
  	    elif lineCounter == NumberOfLines and not 'nsaos' in line:
  	     #take care for non-complete lines
	     if ElementsInLastLine != 0:
  	       for k in range(ElementsInLastLine):
  	          c_mo.append(  CoeffString[0+k*20:20+k*20] )
	     else:	
  	       for k in range(4):
  	          c_mo.append(  CoeffString[0+k*20:20+k*20] )

	     for j in range(nsaosOfOtherMolecule):
  	     #generate field with zeros for the other molecule
  		c_fill_mo.append( '0.00000000000000D-00' )
  	     #now glue eigenvalue, coefficients and zeros together
             if whichMonomer == 1:
  	        eigenvalue.extend(c_mo)	      
  	        eigenvalue.extend(c_fill_mo)
	     else:
  	        eigenvalue.extend(c_fill_mo)
  	        eigenvalue.extend(c_mo)	      
  	     #store complete mo into the mo-vector list	      
  	     vec_mo.append( eigenvalue )
  	     #re-initialize for next pass
  	     c_mo=[]
  	     c_fill_mo=[]
  	     eigenvalue=[]
	     readMo='false'
  moFile.close()

  return vec_mo



def getOrbDim(path):
  """ extract "scfconv" and "nsaos" from the orbital read """
     


  moFile=open(path,'r')
  for line in moFile:
	  #skip comment lines
          if line.find('#') == 0:
	    continue

	  #stop reading when $end statement is reached
   	  if '$end' in line:
	    print "we should never come here!"
     	    break

	  #analyse mo-file header
	  if 'scfconv' in line:
	    cols=line.split('=')
	    str0=cols[1].split(' ')
	    scfconv=int(str0[0])

	  #read size of mo-block
	  if 'nsaos' in line:
	      lineCounter=1
	      readMo='true'
      	      cols=line.split('=')
	      nsaos=int(cols[2])
	      break

  moFile.close()

  return scfconv,nsaos	      
	      
	      
def writeMo(scfconv,nsaos,vec_mo,name):
  """vec_mo is the mo vector field obtained by readOrb, name should be alpha,beta or mos"""
 
  import sys
   
  outFile=open(name,'w')

  

  if name == "alpha":
    outFile.write("$uhfmo_alpha    scfconv=%d   format(4d20.14)\n#generated by merge_mos.py\n#\n" % (scfconv))
  if name == "beta":
    outFile.write("$uhfmo_beta    scfconv=%d   format(4d20.14)\n#generated by merge_mos.py\n#\n" % (scfconv))
  if name == "mos":
    outFile.write("$scfmo    scfconv=%d   format(4d20.14)\n#generated by merge_mos.py\n#\n" % (scfconv))

  ElementsInLastLineNew=nsaos % 4
  for i in range(nsaos):
  #loop over mos
     outFile.write("%6d  a      eigenvalue=%19.14lfD+00   nsaos=%d\n" % (i+1,vec_mo[i][0],nsaos))
     for j in range(nsaos/4):
     #loop over lines
  	outFile.write("%s%s%s%s\n" % (vec_mo[i][1+j*4],vec_mo[i][2+j*4],vec_mo[i][3+j*4],vec_mo[i][4+j*4]))
     if ElementsInLastLineNew > 0:
  	LastLine=[]
  	for k in range(ElementsInLastLineNew):
  	   #loop for elements in last line
  	   LastLine.append(vec_mo[i][k+1+(j+1)*4])
  	str3=''.join(LastLine)
  	outFile.write("%s\n" % (str3))
  outFile.write('$end\n')
  outFile.close()   
  
  
  
  
  