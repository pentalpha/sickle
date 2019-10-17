import sys
import subprocess
import os
from tqdm import tqdm

print("How to use: python trim_all.py [se|pe] [solexa|illumina|sanger] input_dir/ output_dir/ [threads]")
#modes:
#pe (paired-end)
#pes (paired-end single file)
#se (single-end)

def runCommand(cmd):
    print("\t> " + cmd)
    process = subprocess.call(cmd, shell=True)
    return process

def rm_last_part(path, c):
    parts = path.split(c)
    if len(parts) > 1:
        return c.join(parts[:-1])
    else:
        return parts[0]

def file_name(path):
    no_file_extension = rm_last_part(path, ".")
    name = no_file_extension.split("/")[-1]
    return name

def get_files_by_type(dir, extension):
    files = os.listdir(dir)
    return [i for i in files if i.endswith(extension)]

def get_fastqs(path):
    return get_files_by_type(path, ".fq") + get_files_by_type(path, ".fastq")

def get_fastqs_1(path,sep="."):
    return get_files_by_type(path, sep+'1.fq') + get_files_by_type(path, sep+'1.fastq')


def exists(path):
    return os.path.exists(path)

assert(len(sys.argv) == 4)

#print('sys.argv[0] =', sys.argv[0])             
pathname = os.path.dirname(sys.argv[0])        
#print('path =', pathname)
print('full path =', os.path.abspath(pathname))

sickle = os.path.abspath(pathname) + "/sickle"
mode = sys.argv[1]
qual_type = sys.argv[2]
input_dir = sys.argv[3]
output_dir = sys.argv[4]
threads = 32
if(len(sys.argv) >= 6):
    threads = str(sys.argv[5])
max_batch = 512
if(len(sys.argv) >= 7):
    max_batch = str(sys.argv[6])

if(mode == "se"):
    fastqs = get_fastqs(input_dir)
    print("Running sickle se for the following files: \n" + "\n".join(fastqs))
    for i in tqdm(range(len(fastqs))):
        fastq = fastqs[i]
        no_extension = file_name(fastq)
        output = output_dir + "/" + no_extension + ".trim.fastq"
        if not exists(output):
            runCommand(" ".join(["time", sickle, "-t", qual_type,"-f", fastq, "-o", output, "-a", str(threads), "-b", str(max_batch)]))
        else:
            print(output + " already exists, skiping it.")
elif(mode == "pe"):
    sep = "."
    fastqs = get_fastqs_1(input_dir, sep)
    if(len(fastqs) < 2):
        sep = "_"
        fastqs = get_fastqs_1(input_dir,sep)
    print("Running fastp pe for the following files: \n" + "\n".join(fastqs))
    for i in tqdm(range(len(fastqs))):
        fastq_1 = fastqs[i]
        extension = ".fq"
        if ".fastq" in fastq_1:
            extension = ".fastq"
        fastq_2 = fastq_1.rstrip(sep+"1" + extension) + sep+"2" + extension

        fastq1_new = output_dir + "/" +fastq_1.replace(extension, ".trim.fastq")
        fastq2_new = output_dir + "/" +fastq_2.replace(extension, ".trim.fastq")
        singles = fastq2_new.replace("2.trim.fastq", "s.trim.fastq")

        fastq_1 = input_dir + "/" + fastq_1
        fastq_2 = input_dir + "/" + fastq_2

        if not(exists(fastq_1)):
            print("Input " + fastq_1 + " don't exist, finishing.")
            break
        elif not (exists(fastq_2)):
            print("Input " + fastq_2 + " don't exist, finishing.")
            break

        if (not exists(fastq1_new)) and (not exists(fastq2_new)) and (not exists(singles)):
            code = runCommand(" ".join(["time", sickle, "-t", qual_type, "-f", fastq_1, "-r", fastq_2,
                "-o", fastq1_new, "-p", fastq2_new, "-s", singles, "-a", str(threads), "-b", str(max_batch)]))
            if code != 0:
                break
        else:
            print(fastq1_new + " already exists, skiping it.")

else:
    print("There is no '" + mode + "' mode available")