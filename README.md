# min-cc
このレポジトリは下の参考にある自作コンパイラを見ながら書いたプログラムです。
testを通す場合
```bash
docker run --rm -v /Users/akiyama/code/min-cc:/min-cc -w /min-cc mincc make test
```
docker build ./ -t mincc
(docker run --rm -v /Users/akiyama/code/min-cc:/min-cc -w /min-cc mincc uname -a)

## 参考
- https://www.sigbus.info/compilerbook
- https://github.com/rui314/chibicc
