use std::f64::consts::PI;

/// Calculates the DCT for a vector of f64s given a matrix that is n by n.
pub fn dct_type_2(input: &Vec<f64>, n: u32) -> Vec<f64> {
    let mut dct_output: Vec<f64> = vec![0.0; input.len()];

    let coeff = |x: u32| -> f64 {
        if x == 0 {
            1.0 / f64::sqrt(n as f64)
        } else {
            f64::sqrt(2.0 / (n as f64))
        }
    };

    let cos_term = |a, b| f64::cos((2.0 * (a as f64) + 1.0) * (b as f64) * PI / (2.0 * (n as f64)));

    // The core of the dct. u = current x, v = current y, and this iterates the remaining for
    // each pixel. This is an inefficient algorithm. TODO: Replace algorithm
    for v in 0..n {
        for u in 0..n {
            let mut sum = 0.0;

            for y in 0..n {
                for x in 0..n {
                    let current_val = input[(n * y + x) as usize];
                    sum += current_val * cos_term(y, v) * cos_term(x, u);
                }
            }

            dct_output[(n * v + u) as usize] = sum * coeff(u) * coeff(v);
        }
    }

    dct_output
}
