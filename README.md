# min-cc

```bash
docker build ./ -t mincc
docker run --rm -v /Users/akiyama/code/min-cc:/min-cc -w /min-cc mincc xxxxx
(docker run --rm -v /Users/akiyama/code/min-cc:/min-cc -w /min-cc mincc uname -a)
docker-compose up -d
docker-compose exec cc bash
```