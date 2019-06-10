function sequenceSorter(a, b) {
  return a.sequence.rarity < b.sequence.rarity;
}

function sequenceMatch(props, name) {
  let names = name.toLowerCase().split(/[- ]/).map(x => x.trim());
  return props.every(p => names.includes(p));
}

function filterSequences(type, sequences, props) {
  let filtered = [];

  for (let i = 0, l = sequences.length; i < l; i++) {
    let sequence = sequences[i],
      name = sequence.name.split('-')[0].replace(/\d/g, '').trim().toLowerCase();

    if (name === type && sequenceMatch(props, sequence.name)) {
      filtered.push({sequence, index: i});
    }
  }

  return filtered;
}

function selectSequence(type, original, animProps) {
  let props = animProps ? animProps.split(',').filter(x => x.length).map(p => p.toLowerCase()) : [];
  let sequences = filterSequences(type, original, props);
  if (!sequences.length) {
    props.push(type);
    let first = original.findIndex(x => sequenceMatch(props, x.name));
    if (first >= 0) {
      sequences.push({sequence: original[first], index: first});
    }
  }

  sequences.sort(sequenceSorter);

  for (var i = 0, l = sequences.length; i < l; i++) {
    var sequence = sequences[i].sequence;
    let rarity = sequence.rarity;

    if (rarity === 0) {
      break;
    }

    if (Math.random() * 10 > rarity) {
      return sequences[i];
    }
  }

  let sequencesLeft = sequences.length - i;
  let random = i + Math.floor(Math.random() * sequencesLeft);
  var sequence = sequences[random];

  return sequence;
}

function standSequence(target, animProps) {
  let sequences = target.model.sequences;
  let sequence = selectSequence('stand', sequences, animProps);

  if (sequence) {
    target.setSequence(sequence.index);
  }
};

export default function standOnRepeat(target, animProps) {
  target.model.whenLoaded()
    .then((model) => {
      standSequence(target, animProps);
      target.on('seqend', () => standSequence(target, animProps));
    });
};
