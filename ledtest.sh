
    #!/bin/bash
    # Basic while loop
    date
    counter=1
    while [ $counter -le 255 ]
    do
      echo $counter
      owwrite /EB.54594D454BFF/btn-backlight $counter
      ((counter++))
    done
    while [ $counter -gt 1 ]
    do
      echo $counter
      owwrite /EB.54594D454BFF/btn-backlight $counter
      ((counter--))
    done
    date
    echo All done
