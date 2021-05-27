# -*- coding: utf-8 -*-

from act import Act
from sample.projectcode import ProjectCode
from utils.inout import *


class BCBuilder(Act):

    def start(self):
        projects_dir = join_path(self.arguments.data_dir, self.arguments.projects_dir)
        # print self.arguments
        dir_names = get_directories(projects_dir)
        # print "[*] dir_names: \n", dir_names

        for dir_name in dir_names:
            for project_name in self.arguments.projects:
                if get_basename(dir_name) == project_name or len(self.arguments.projects) == 0:
                    print '[*] Analyzing {}'.format(get_basename(dir_name))
                    project_code = ProjectCode(project_dir=dir_name, arguments=self.arguments)
                    print "[*] self.arguments.prepare: ", self.arguments.prepare
                    if self.arguments.prepare: ## self.arguments.prepare 为true表示给出了bitcode
                        project_code.prepare_bc()
                    else:
                        project_code.retrieve_bc()
                    # project_code.link_bc_files()
