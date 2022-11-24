
    #!/bin/bash
    # Basic while loop
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
    echo All done
