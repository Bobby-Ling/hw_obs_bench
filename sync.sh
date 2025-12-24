#!/bin/bash

# ssh root@ecs-bobby

# rsync -avz --dry-run -P --exclude-from=.gitignore -e 'ssh -p 22' ./ root@ecs-bobby:/root/fio
rsync -avz             -P --exclude-from=.gitignore -e 'ssh -p 22' ./ root@ecs-bobby:/root/fio
