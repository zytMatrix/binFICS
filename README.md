# 源码相似度检测

## 1. 文件所在目录

`datasets_v1`中包含对应版本的源码和保存函数相似度值的`csv`文件：

```
# ls datasets_v1
boringssl-version_for_cocoapods_9.0.gz  openssl_boring_1h.csv  openssl-OpenSSL_1_0_1f.tar.gz openssl_boring_times.csv
```

## 2. 函数相似度比较

### 相似度结果的csv文件中的格式：

```
## 表头分别是：项目1、项目2、函数相似度
openssl_101f	boringssl_0.9	sim_value 
cpy_univ	cpy_bmp		1
PEM_read_bio_PKCS8	PEM_read_bio_PKCS8	1
SSL_get_fd	SSL_num_renegotiations		1
... ...
PEM_write_bio_PUBKEY	i2d_PrivateKey_bio	0.95
... ...
BN_mod_sqr	BN_mod_exp	0.914039336
... ...
```

### 函数源码对比

以上面的函数`BN_mod_sqr` 和 `BN_mod_exp`为例：

`openssl_101f`中的`BN_mod_sqr:`

```c
int BN_mod_lshift1(BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx){
	if (!BN_lshift1(r, a)) return 0;
	bn_check_top(r);
	return BN_nnmod(r, r, m, ctx);
}
```

`boringssl_0.9`中的`BN_mod_exp`

```c
int BN_mod_exp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, const BIGNUM *m, BN_CTX *ctx) {
  if (BN_is_odd(m)) {
    return BN_mod_exp_mont(r, a, p, m, ctx, NULL);
  }

  return mod_exp_recp(r, a, p, m, ctx);
}
```

