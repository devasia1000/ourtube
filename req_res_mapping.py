#! /usr/bin/python
import os
counter=1
req_mapping = dict()
res_mapping = dict()
os.system("rm -rf mapping")
os.mkdir("mapping")
for file_name in os.listdir("logs"):
  if ("req" in file_name):
      req_mapping[file_name.strip(".req")] = open("logs/"+file_name, "rb").read();
  elif ("res" in file_name):
      res_mapping[file_name.strip(".res")] = open("logs/"+file_name, "rb").read();
  else :
      print "Invalid, exiting"
      exit(5)

for pid in req_mapping:
    requests = ["GET /" + rec for rec in req_mapping[pid].split("GET /") if rec != ''];
    responses = ["HTTP/1.1 " + rec for rec in res_mapping[pid].split("HTTP/1.1 ") if rec != ''];

    print pid, "Requests ", len(requests), "Responses ", len(responses)
    assert(len(requests) == len(responses))
    for i in range(0, len(requests)):
      req_fh = open("mapping/"+str(counter)+".object","w");
      print>>req_fh, requests[i], responses[i],
      counter += 1
