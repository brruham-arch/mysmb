for v in 0.10.9+6e6c44045 0.10.7+bc794d341 ""; do
  if [ -n "$v" ]; then
    curl -fsSL https://cli.moonbitlang.com/install/unix.sh | bash -s "$v" && break
  else
    curl -fsSL https://cli.moonbitlang.com/install/unix.sh | bash && break
  fi
done
