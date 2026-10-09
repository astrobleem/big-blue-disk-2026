from pathlib import Path
import struct,hashlib,json,math
HERE=Path(__file__).parent
SECTOR=512;SPC=2;SIZE=1440*512;ROOTSTART=7*512;DATASTART=14*512;CLUSTER=1024
def name83(name):
 p=name.split('.');assert len(p)<=2 and 1<=len(p[0])<=8,name
 assert len(p)==1 or 1<=len(p[1])<=3,name
 assert all(c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in ''.join(p)),name
 return (p[0].ljust(8)+(p[1] if len(p)>1 else '').ljust(3)).encode('ascii')
def entry(name,attr,cluster,size):
 e=bytearray(32);e[:11]=name83(name);e[11]=attr
 struct.pack_into('<HHHHHI',e,14,0x6000,0x5d47,0x5d47,0,0x6000,0)
 struct.pack_into('<HHHI',e,22,0x6000,0x5d47,cluster,size)
 return bytes(e)
def build():
 source=HERE/'runtime';files=sorted(p for p in source.rglob('*') if p.is_file())
 assert (source/'BBD2026/GO.EXE').exists()
 dirs=sorted((p for p in source.rglob('*') if p.is_dir()),key=lambda p:(len(p.relative_to(source).parts),str(p)))
 image=bytearray(SIZE);fat=[0]*715;fat[0]=0xff9;fat[1]=0xfff;nextcl=2;chains={}
 def allocate(path,count):
  nonlocal nextcl
  count=max(1,count);assert nextcl+count<=715,'Disk full'
  chain=list(range(nextcl,nextcl+count));nextcl+=count
  for a,b in zip(chain,chain[1:]):fat[a]=b
  fat[chain[-1]]=0xfff;chains[path]=chain
 for p in dirs:allocate(p,1)
 for p in files:
  name83(p.name);allocate(p,math.ceil(p.stat().st_size/CLUSTER))
 for p in files:
  b=p.read_bytes()
  for i,cl in enumerate(chains[p]):
   at=DATASTART+(cl-2)*CLUSTER;image[at:at+CLUSTER]=b[i*CLUSTER:(i+1)*CLUSTER].ljust(CLUSTER,b'\0')
 for d in [source]+dirs:
  children=sorted(d.iterdir(),key=lambda p:(p.is_file(),p.name))
  records=[]
  if d!=source:
   records=[entry('DOT',0x10,chains[d][0],0),entry('UP',0x10,0 if d.parent==source else chains[d.parent][0],0)]
   records[0]=b'.          '+records[0][11:]
   records[1]=b'..         '+records[1][11:]
  else:records=[entry('BBD2026',8,0,0)]
  for p in children:
   if not p.is_dir() and not p.is_file():continue
   records.append(entry(p.name,0x10 if p.is_dir() else 0x20,chains[p][0],0 if p.is_dir() else p.stat().st_size))
  buf=b''.join(records)
  if d==source:assert len(buf)<=112*32;image[ROOTSTART:ROOTSTART+len(buf)]=buf
  else:
   assert len(buf)<=CLUSTER,'Directory full'
   at=DATASTART+(chains[d][0]-2)*CLUSTER;image[at:at+len(buf)]=buf
 f=bytearray(1536)
 for i,n in enumerate(fat):
  off=i+i//2
  if i%2:f[off]=(f[off]&15)|((n&15)<<4);f[off+1]=(n>>4)&255
  else:f[off]=n&255;f[off+1]=(f[off+1]&240)|((n>>8)&15)
 image[512:2048]=f;image[2048:3584]=f
 boot=bytearray(512);boot[:3]=b'\xeb\x3c\x90';boot[3:11]=b'BB2026  '
 struct.pack_into('<HBHBHHBHHHII',boot,11,512,2,1,2,112,1440,0xf9,3,9,2,0,0)
 boot[36]=0;boot[38]=0x29;struct.pack_into('<I',boot,39,0x20261007)
 boot[43:54]=b'BBD2026TEST';boot[54:62]=b'FAT12   '
 code=bytearray(b'\x31\xc0\x8e\xd8\xbe\x00\x00\xac\x84\xc0\x74\x09\xb4\x0e\xbb\x07\x00\xcd\x10\xeb\xf2\xfa\xf4\xeb\xfd')
 struct.pack_into('<H',code,5,0x7c00+62+len(code))
 msg=b'Data disk: boot DOS first, then run BBD2026\\GO.\r\n\0'
 boot[62:62+len(code)]=code;boot[62+len(code):62+len(code)+len(msg)]=msg;boot[510:]=b'\x55\xaa';image[:512]=boot
 dest=HERE/'ISSUE26.IMG';dest.write_bytes(image);(HERE/'ISSUEQA.IMG').write_bytes(image)
 expected={p.relative_to(source).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
 found=read_image(dest);assert set(found)==set(expected)
 assert all(hashlib.sha256(b).hexdigest()==expected[n] for n,b in found.items())
 report={'image_bytes':len(image),'file_count':len(files),'payload_bytes':sum(p.stat().st_size for p in files),
  'directories':len(dirs),'allocated_clusters':nextcl-2,'allocated_data_bytes':(nextcl-2)*1024,
  'free_clusters':715-nextcl,'free_data_bytes':(715-nextcl)*1024,'data_capacity_bytes':713*1024,
  'geometry':'512B sectors,2sectors/cluster,1440sectors,2heads,9sectors/track,FAT12',
  'sha256':hashlib.sha256(image).hexdigest(),'files':expected}
 (HERE/'IMAGE.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='files'}))
def read_image(path):
 image=Path(path).read_bytes();assert len(image)==SIZE
 assert struct.unpack_from('<H',image,11)[0]==512 and image[13]==2
 assert image[512:2048]==image[2048:3584],'FAT copies differ'
 fat=image[512:2048];found={};seen=set()
 def chain(cl):
  chunks=[];local=set()
  while cl<0xff8:
   assert 2<=cl<715 and cl not in local and cl not in seen,'Invalid/shared/cyclic chain'
   local.add(cl);seen.add(cl)
   at=DATASTART+(cl-2)*CLUSTER;chunks.append(image[at:at+CLUSTER])
   off=cl+cl//2;v=fat[off]|(fat[off+1]<<8);cl=(v>>4 if cl&1 else v&0xfff)
  return b''.join(chunks)
 def directory(buf,parent):
  for i in range(0,len(buf),32):
   e=buf[i:i+32]
   if not e or e[0]==0:break
   if e[0]==229 or e[11]&8 or e[:1]==b'.':continue
   name=e[:8].decode('ascii').rstrip();ext=e[8:11].decode('ascii').rstrip()
   if ext:name+='.'+ext
   cl=struct.unpack_from('<H',e,26)[0];size=struct.unpack_from('<I',e,28)[0]
   content=chain(cl) if cl>=2 else b'';path=parent+name
   if e[11]&16:directory(content,path+'/')
   else:found[path]=content[:size]
 directory(image[ROOTSTART:DATASTART],'');return found
if __name__=='__main__':build()

