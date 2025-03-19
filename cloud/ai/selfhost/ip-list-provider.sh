#!/bin/bash
echo "Prod IPs:"
#             translate-prod           stt-prod              stt-diogenes           stt-horatius            stt-aurelius             stt-no-lm
for ig in "cl1cqmbf1n0lafiip5a5" "cl1pem0k1lrdg7e8vmsn" "cl12su4k6c4832c2c8oh" "cl1c0pjg38920i2jtgtl" "cl16nes719gjur1biac9" "cl1c49b22i5lv8jsrmg3"
  do yc compute ig list-instances ${ig} | grep " 2a02:" | cut -d "|" -f 5
done

echo "Private Prod IPs:"
#               stt-diogenes          stt-anaximander          stt-no-lm        stt-aurelius-hotfix1     services-proxy
for ig in "cl1rchbhrbubnvlmua8d" "cl17s6oom7331cocj4q3" "cl1jd62hmclcr0imr4eh" "cl1n9ru64pkcf7gqdkvi" "cl1kas2j7couoglqbim1"
  do yc compute ig list-instances ${ig} | grep " 2a02:" | cut -d "|" -f 5
done

echo "Preprod IPs:"
#               stt-preprod
for ig in "cl136k54g0n5smkfee13"
  do yc compute ig list-instances ${ig} | grep " 2a02:" | cut -d "|" -f 5
done
