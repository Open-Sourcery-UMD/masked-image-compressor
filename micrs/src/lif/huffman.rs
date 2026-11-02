use crate::lif::huffman::Node::Leaf;
use std::collections::HashMap;
use std::hash::Hash;

/// Represents a Huffman tree (the only rule is that everything must be a leaf node)
pub(super) struct HuffmanTree<T: Eq + Hash> {
    root: Option<Node<T>>,
}

/// Type alias for an optional Node<T> reference
type NodeRef<T> = Option<Box<Node<T>>>;

/// Represents a node in the Huffman tree
enum Node<T: Eq + Hash> {
    Internal(InternalNode<T>),
    Leaf(LeafNode<T>),
}

/// Represents an internal node in the Huffman tree
struct InternalNode<T: Eq + Hash> {
    frequency: usize,
    left: NodeRef<T>,
    right: NodeRef<T>,
}

/// Represents a leaf node in the Huffman tree
struct LeafNode<T: Eq + Hash> {
    frequency: usize,
    data: T,
}

/// Default trait implementation for HuffmanTree
impl<T: Eq + Hash> Default for HuffmanTree<T> {
    fn default() -> Self {
        HuffmanTree { root: None }
    }
}

/// Default trait implementation for InternalNode
impl<T: Eq + Hash> Default for InternalNode<T> {
    fn default() -> Self {
        InternalNode {
            frequency: 0,
            left: None,
            right: None,
        }
    }
}

impl<T: Eq + Hash> LeafNode<T> {
    /// Creates a new LeafNode
    fn new(data: T, frequency: usize) -> Self {
        LeafNode { data, frequency }
    }
}

impl<T: Eq + Hash> HuffmanTree<T> {
    /// Returns a new HuffmanTree, if it was successfully created or otherwise returns None
    pub(super) fn new(consumable: impl Into<Vec<T>>) -> Option<Self> {
        let elements: Vec<T> = consumable.into();

        if elements.is_empty() {
            return None;
        }

        let mut hashmap = HashMap::new();
        let tree = HuffmanTree::default();

        for e in &elements {
            if !hashmap.contains_key(e) {
                hashmap.insert(e, 1usize);
            } else {
                let mut_ref = hashmap.get_mut(e).unwrap();
                *mut_ref += 1;
            }
        }

        let mut hashmap_sorted: Vec<(&&T, &usize)> = hashmap.iter().collect();
        hashmap_sorted.sort_by(|a, b| a.1.cmp(b.1));

        let mut root: Option<InternalNode<T>> = Some(InternalNode::default());

        let mut i = 0;

        while i < hashmap_sorted.len() {
            let under = root.take().unwrap();

        }

        Some(tree)
    }
}
