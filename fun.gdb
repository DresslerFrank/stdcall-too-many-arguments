br busyloop
set $count = 0
commands
    silent
    printf "busyloop #%u\n", ++$count
    if $count >= 20
        quit
    end
    continue
end
run
