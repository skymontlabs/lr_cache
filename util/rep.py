import numpy as np

def siegelslopes_pythran(y, x, method):
    # each x and y value will subtract another one. (n^2)
    deltax = np.expand_dims(x, 1) - x
    deltay = np.expand_dims(y, 1) - y

    print(x)
    print(y)
    #print(np.expand_dims(x, 1))
    #print(np.expand_dims(y, 1))
    print(deltax)
    print(deltay)
    slopes, intercepts = [], []

    for j in range(len(x)):
        # indices of the nonzero elements of deltax[j,:]
        # this means we are finding the difference betwen value xi and all the other x values
        id_nonzero, = np.nonzero(deltax[j, :])
        print(id_nonzero)
        print(deltax[j, id_nonzero])
        print(deltay[j, id_nonzero])


        # calculate the slope of the line of each point pair
        slopes_j = deltay[j, id_nonzero] / deltax[j, id_nonzero] 
        # find median of all the slopes
        medslope_j = np.median(slopes_j) 
        slopes.append(medslope_j)

        if method == 'separate':
            z = y*x[j] - y[j]*x
            medintercept_j = np.median(z[id_nonzero] / deltax[j, id_nonzero])
            intercepts.append(medintercept_j)


    medslope = np.median(np.asarray(slopes))
    if method == "separate":
        medinter = np.median(np.asarray(intercepts))
    else:
        medinter = np.median(y - medslope*x)

    return medslope, medinter


def siegelslopes(y, x=None, method="hierarchical"):
    r"""
    Computes the Siegel estimator for a set of points (x, y).
    `siegelslopes` implements a method for robust linear regression
    using repeated medians (see [1]_) to fit a line to the points (x, y).
    The method is robust to outliers with an asymptotic breakdown point
    of 50%.
    Parameters
    ----------
    y : array_like
        Dependent variable.
    x : array_like or None, optional
        Independent variable. If None, use ``arange(len(y))`` instead.
    method : {'hierarchical', 'separate'}
        If 'hierarchical', estimate the intercept using the estimated
        slope ``slope`` (default option).
        If 'separate', estimate the intercept independent of the estimated
        slope. See Notes for details.
    Returns
    -------
    result : ``SiegelslopesResult`` instance
        The return value is an object with the following attributes:
        slope : float
            Estimate of the slope of the regression line.
        intercept : float
            Estimate of the intercept of the regression line.
    See Also
    --------
    theilslopes : a similar technique without repeated medians
    Notes
    -----
    With ``n = len(y)``, compute ``m_j`` as the median of
    the slopes from the point ``(x[j], y[j])`` to all other `n-1` points.
    ``slope`` is then the median of all slopes ``m_j``.
    Two ways are given to estimate the intercept in [1]_ which can be chosen
    via the parameter ``method``.
    The hierarchical approach uses the estimated slope ``slope``
    and computes ``intercept`` as the median of ``y - slope*x``.
    The other approach estimates the intercept separately as follows: for
    each point ``(x[j], y[j])``, compute the intercepts of all the `n-1`
    lines through the remaining points and take the median ``i_j``.
    ``intercept`` is the median of the ``i_j``.
    The implementation computes `n` times the median of a vector of size `n`
    which can be slow for large vectors. There are more efficient algorithms
    (see [2]_) which are not implemented here.
    For compatibility with older versions of SciPy, the return value acts
    like a ``namedtuple`` of length 2, with fields ``slope`` and
    ``intercept``, so one can continue to write::
        slope, intercept = siegelslopes(y, x)
    References
    ----------
    .. [1] A. Siegel, "Robust Regression Using Repeated Medians",
           Biometrika, Vol. 69, pp. 242-244, 1982.
    .. [2] A. Stein and M. Werman, "Finding the repeated median regression
           line", Proceedings of the Third Annual ACM-SIAM Symposium on
           Discrete Algorithms, pp. 409-413, 1992.
    Examples
    --------
    >>> import numpy as np
    >>> from scipy import stats
    >>> import matplotlib.pyplot as plt
    >>> x = np.linspace(-5, 5, num=150)
    >>> y = x + np.random.normal(size=x.size)
    >>> y[11:15] += 10  # add outliers
    >>> y[-5:] -= 7
    Compute the slope and intercept.  For comparison, also compute the
    least-squares fit with `linregress`:
    >>> res = stats.siegelslopes(y, x)
    >>> lsq_res = stats.linregress(x, y)
    Plot the results. The Siegel regression line is shown in red. The green
    line shows the least-squares fit for comparison.
    >>> fig = plt.figure()
    >>> ax = fig.add_subplot(111)
    >>> ax.plot(x, y, 'b.')
    >>> ax.plot(x, res[1] + res[0] * x, 'r-')
    >>> ax.plot(x, lsq_res[1] + lsq_res[0] * x, 'g-')
    >>> plt.show()
    """
    if method not in ['hierarchical', 'separate']:
        raise ValueError("method can only be 'hierarchical' or 'separate'")
    y = np.asarray(y).ravel()
    x = np.asarray(x, dtype=float).ravel()
    if len(x) != len(y):
        raise ValueError("Incompatible lengths ! (%s<>%s)" %
                         (len(y), len(x)))
    dtype = np.result_type(x, y, np.float32)  # use at least float32
    y, x = y.astype(dtype), x.astype(dtype)
    return siegelslopes_pythran(y, x, method)

if __name__ == '__main__':
    x = np.array([1,2,3,4,5,6,7,8,9])
    y = np.array([4,6,9,12,15,18,21,25,28])
    print(siegelslopes(y,x))