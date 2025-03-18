import library.python.testing.recipe
import library.recipes.docker_compose.lib as docker_compose_lib
import yatest.common
import time


def start_healthcheck(argv):
    docker_compose_lib.start(argv)
    yaml, cwd = docker_compose_lib.get_compose_file_and_cwd(argv)
    docker_compose_lib.logger.info("Running health checks")
    stdout_path = yatest.common.work_path('simulate_stdout.txt')
    for i in range(7):
        healthy = True
        if i > 0:
            docker_compose_lib.logger.info("Sleeping for 10 seconds. Attempt %d" % i)
            time.sleep(10)
        with open(stdout_path, "w") as output_file:
            yatest.common.execute([docker_compose_lib.get_docker_compose(), "-f", yaml, "ps"], stdout=output_file, cwd=cwd)
        with open(stdout_path, "r") as output_file:
            content = output_file.readlines()
            for line in content:
                if line.find("Up (health: starting)") != -1:
                    healthy = False
        if healthy:
            break

if __name__ == "__main__":
    library.python.testing.recipe.declare_recipe(start_healthcheck, docker_compose_lib.stop)
