# BPLCG

- **Category:** Crypto
- **Solves:** 0
- **Tag:** ZKP

## Description

Have you ever heard of Bulletproofs?

## Details

We are given an implementation of [Bulletproofs' Range Proof](https://eprint.iacr.org/2017/1066.pdf) that chooses the challenges using an LCG seeded on the committed values. The objective is to forge a valid range proof for a commitment $V = v \cdot G + \gamma \cdot H$ to an integer $v$ not in range $[2^n]$.

## Solution

Maybe there are multiple paths that lead to a forgery, here we will see one that uses the challenges $y$ and $z$.

The challenges $y$ and $z$ are peculiar since they are the only ones that are generated **sequentially** without any commitment in between. Let $X$ be the LCG state before sampling $y$, then $y = X$ and $z = a \cdot X + b$, where $a, b$ are the LCG parameters. This means that the checks that require $y$ and $z$ to be independent are now faulty.

Essentially the idea of Bulletproofs is (1) express $v$ is its binary representation, which has length at most $n$, (2) check that the representation is binary using an inner product argument. Thus, given $\mathbf{a}_L$ binary representation of $v$ and $\mathbf{a}_R$, we check that

$$\langle \mathbf{a}_L, \mathbf{2}^n \rangle = v \quad \text{and} \quad \mathbf{a}_L \circ \mathbf{a}_R = \mathbf{0}^n \quad \text{and} \quad \mathbf{a}_R = \mathbf{a}_L - \mathbf{1}^n$$

where $\circ$ is the point-wise product.

In the actual protocol, the above checks are **batched** using $y$ and $z$. On the other hand, zero-knowledge is achieved considering an additional challenge $x$ that multiplies the masks $\mathbf{s}_L$ and $\mathbf{s}_R$. We do not consider this last part, as the checks are disjoint thanks to $x$.

Thus, the main check we are interested in, is the one given in Equation (39)

$$\Big\langle \mathbf{a}_L - z \cdot \mathbf{1}^n, \mathbf{y}^n \circ \big( \mathbf{a}_R + z \cdot \mathbf{1}^n \big) + z^2 \cdot \mathbf{2}^n \Big\rangle = z^2 \cdot v + \delta(y, z),$$

or, equivalently,

$$\langle \mathbf{l}_0, \mathbf{r}_0 \rangle = t_0 = z^2 \cdot v + \delta(y, z).$$

Our $y$ and $z$ are affine transformations of $X$, so the above becomes

$$\Big\langle \mathbf{a}_L - (a \cdot X + b) \cdot \mathbf{1}^n, \mathbf{X}^n \circ \big( \mathbf{a}_R + (a \cdot X + b) \cdot \mathbf{1}^n \big) + (a \cdot X + b)^2 \cdot \mathbf{2}^n \Big\rangle = (a \cdot X + b)^2 \cdot v + \delta(X, a \cdot X + b),$$

which has degree at most $n$ in $X$. Since $X$ is unknown we need to satisfy the equation for each coefficient of $X^i$, for all $i = 0, \dots, n$. Thus, we need to find $v$, $\mathbf{a}_L$ and $\mathbf{a}_R$ that satisfy $n + 1$ quadratic equations.

Although we are working with quadratic equations, the quadratic terms above are only given by the monomials $\mathbf{a}_L \circ \mathbf{a}_R$. Thus, by fixing the $\mathbf{a}_L$ to random values we obtain a linear system of $n + 1$ equations in $v$ and $\mathbf{a}_R$, which with high probability will give a solution with $v \notin [2^n]$.

Now, it only remains to set $v$, $\mathbf{a}_L$ and $\mathbf{a}_R$ to the values obtained above, when computing the range proof. The proof should verify in remote and give us the flag. See the actual implementation in [solve.py](./src/solve.py) for more details.

