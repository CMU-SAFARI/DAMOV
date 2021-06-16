import sys
import os
import errno

os.chdir("../workloads")
PIM_ROOT = os.getcwd() +"/"
os.chdir("../simulator") 
ROOT = os.getcwd() +"/"

def mkdir_p(directory):
    try:
        os.makedirs(directory)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(directory):
            pass
        else:
            raise

def create_host_configs_no_prefetch(benchmark, application, function, command, version):
    number_of_cores = [1, 4, 16, 64, 256]

    for cores in number_of_cores:
        mkdir_p(ROOT+"config_files/host_"+version+"/no_prefetch/"+benchmark+"/"+str(cores)+"/")

    for cores in number_of_cores:
        mkdir_p(ROOT+"zsim_stats/host_"+version+"/no_prefetch/"+str(cores)+"/")

    for cores in number_of_cores:
        with open(ROOT+"templates/template_host_"+version+".cfg", "r") as ins:
            config_file = open(ROOT+"config_files/host_"+version+"/no_prefetch/"+benchmark+"/"+str(cores)+"/"+application+"_"+function+".cfg","w")
            for line in ins:
                line = line.replace("NUMBER_CORES", str(cores))
                line = line.replace("STATS_PATH", "zsim_stats/host_"+version+"/no_prefetch/"+str(cores)+"/"+benchmark+"_"+application+"_"+function)
                line = line.replace("COMMAND_STRING", "\"" + command + "\";")
                line = line.replace("THREADS", str(cores))
                line = line.replace("PIM_ROOT",PIM_ROOT)

                config_file.write(line)
            config_file.close()
        ins.close()

def create_host_configs_prefetch(benchmark, application, function, command, version):
    number_of_cores = [1, 4, 16, 64, 256]

    for cores in number_of_cores:
        mkdir_p(ROOT+"config_files/host_"+version+"/prefetch/"+benchmark+"/"+str(cores)+"/")

    for cores in number_of_cores:
        mkdir_p(ROOT+"zsim_stats/host_"+version+"/prefetch/"+str(cores)+"/")

    for cores in number_of_cores:
        with open(ROOT+"templates/template_host_prefetch_"+version+".cfg", "r") as ins:
            config_file = open(ROOT+"config_files/host_"+version+"/prefetch/"+benchmark+"/"+str(cores)+"/"+application+"_"+function+".cfg","w")
            for line in ins:
                line = line.replace("NUMBER_CORES", str(cores))
                line = line.replace("STATS_PATH", "zsim_stats/host_"+version+"/prefetch/"+str(cores)+"/"+benchmark+"_"+application+"_"+function)
                line = line.replace("COMMAND_STRING", "\"" + command + "\";")
                line = line.replace("THREADS", str(cores))
                line = line.replace("PIM_ROOT",PIM_ROOT)

                config_file.write(line)
            config_file.close()
        ins.close()

def create_pim_configs(benchmark, application, function, command, version):
    number_of_cores = [1, 4, 16, 64, 256]

    for cores in number_of_cores:
        mkdir_p(ROOT+"config_files/pim_"+version+"/"+benchmark+"/"+str(cores)+"/")

    for cores in number_of_cores:
        mkdir_p(ROOT+"zsim_stats/pim_"+version+"/"+str(cores)+"/")

    for cores in number_of_cores:
        with open(ROOT+"templates/template_pim_"+version+".cfg", "r") as ins:
            config_file = open(ROOT+"config_files/pim_"+version+"/"+benchmark+"/"+str(cores)+"/"+application+"_"+function+".cfg","w")
            for line in ins:
                line = line.replace("NUMBER_CORES", str(cores))
                line = line.replace("STATS_PATH", "zsim_stats/pim_"+version+"/"+str(cores)+"/"+benchmark+"_"+application+"_"+function)
                line = line.replace("COMMAND_STRING", "\"" + command + "\";")
                line = line.replace("THREADS", str(cores))
                line = line.replace("PIM_ROOT",PIM_ROOT)

                config_file.write(line)
            config_file.close()
        ins.close()


if(len(sys.argv) < 2):
    print "Usage python generate_config_files.py command_file"
    print "command_file: benckmark,applicationm,function,command"
    exit(1)

with open(sys.argv[1], "r") as command_file:
    for line in command_file:
        line = line.split(",")
        benchmark = line[0]
        application = line[1]
        function = line[2]
        command = line[3]
        print line
        command = command.replace('\n','')

        ### Fixed LLC Size 
        create_host_configs_no_prefetch(benchmark, application, function, command, "inorder")
        create_host_configs_prefetch(benchmark, application, function, command, "inorder")
        create_pim_configs(benchmark, application, function, command,"inorder")

        ### Fixed LLC Size 
        create_host_configs_no_prefetch(benchmark, application, function, command, "ooo")
        create_host_configs_prefetch(benchmark, application, function, command, "ooo")
        create_pim_configs(benchmark, application, function, command,"ooo")

        create_host_configs_no_prefetch(benchmark, application, function, command, "accelerator")
        create_host_configs_prefetch(benchmark, application, function, command, "accelerator")
        create_pim_configs(benchmark, application, function, command,"accelerator")

